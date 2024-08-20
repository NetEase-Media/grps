/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/08
 * Brief  global configs.
 */

#include "global_config.h"

#include <iostream>
#include <regex>

#define YAML_TRY_EXTRACT(yaml_node, conf_name, DataType, output)                                          \
  try {                                                                                                   \
    output = yaml_node[#conf_name].as<DataType>();                                                        \
  } catch (const std::exception& e) {                                                                     \
    std::cerr << "Failed to parse conf, conf_name: " << #conf_name << ", err: " << e.what() << std::endl; \
    return false;                                                                                         \
  }

namespace netease::grps {
bool GlobalConfig::Load(const std::string& server_conf_path, const std::string& inference_conf_path) {
  try {
    return LoadServerConf(server_conf_path) && LoadInferenceConf(inference_conf_path);
  } catch (const std::exception& e) {
    std::cerr << "[server.yml] Failed to load config file: " << e.what() << std::endl;
    return false;
  }
}

bool GlobalConfig::LoadServerConf(const std::string& conf_path) {
  YAML::Node server_conf = YAML::LoadFile(conf_path);
  if (!server_conf || server_conf.IsNull() || !server_conf.IsMap()) {
    std::cerr << "[server.yml] Failed to load server config file: " << conf_path << std::endl;
    return false;
  }

  auto interface_conf = server_conf["interface"];
  if (!interface_conf || interface_conf.IsNull() || !interface_conf.IsMap()) {
    std::cerr << "[server.yml] Interface conf is invalid." << std::endl;
    return false;
  }
  YAML_TRY_EXTRACT(interface_conf, framework, std::string, server_config_.interface.framework);
  YAML_TRY_EXTRACT(interface_conf, host, std::string, server_config_.interface.host);
  YAML_TRY_EXTRACT(interface_conf, port, std::string, server_config_.interface.port);
  server_config_._is_set.interface = true;
  auto customized_predict_http = interface_conf["customized_predict_http"];
  if (customized_predict_http && !customized_predict_http.IsNull() && customized_predict_http.IsMap()) {
    YAML_TRY_EXTRACT(customized_predict_http, path, std::string, server_config_.interface.customized_predict_http.path);
    // use regex check format.
    std::regex customized_path_regex(R"(^\/[a-zA-Z0-9_\-\/]+$)");
    if (!std::regex_match(server_config_.interface.customized_predict_http.path, customized_path_regex)) {
      std::cerr << "[server.yml] Server customized_predict_http path is invalid." << std::endl;
      return false;
    }
    YAML_TRY_EXTRACT(customized_predict_http, customized_body, bool,
                     server_config_.interface.customized_predict_http.customized_body);
    server_config_._is_set.customized_predict_http = true;
    auto streaming_ctrl = customized_predict_http["streaming_ctrl"];
    if (streaming_ctrl && !streaming_ctrl.IsNull() && streaming_ctrl.IsMap()) {
      if (streaming_ctrl["ctrl_mode"] && streaming_ctrl["ctrl_mode"].IsNull()) {
        server_config_.interface.customized_predict_http.streaming_ctrl.ctrl_mode =
          ServerConfig::StreamingCtrlMode::kQueryParam;
      } else {
        std::string ctrl_mode;
        YAML_TRY_EXTRACT(streaming_ctrl, ctrl_mode, std::string, ctrl_mode);
        if (ctrl_mode.empty() || ctrl_mode == "query_param") {
          server_config_.interface.customized_predict_http.streaming_ctrl.ctrl_mode =
            ServerConfig::StreamingCtrlMode::kQueryParam;
        } else if (ctrl_mode == "header_param") {
          server_config_.interface.customized_predict_http.streaming_ctrl.ctrl_mode =
            ServerConfig::StreamingCtrlMode::kHeaderParam;
        } else if (ctrl_mode == "body_param") {
          server_config_.interface.customized_predict_http.streaming_ctrl.ctrl_mode =
            ServerConfig::StreamingCtrlMode::kBodyParam;
        } else {
          std::cerr << "[server.yml] Server customized_predict_http streaming_ctrl ctrl_mode is invalid." << std::endl;
          return false;
        }
      }
      if (streaming_ctrl["ctrl_key"] && streaming_ctrl["ctrl_key"].IsNull()) {
        server_config_.interface.customized_predict_http.streaming_ctrl.ctrl_key = "streaming";
      } else {
        YAML_TRY_EXTRACT(streaming_ctrl, ctrl_key, std::string,
                         server_config_.interface.customized_predict_http.streaming_ctrl.ctrl_key);
        if (server_config_.interface.customized_predict_http.streaming_ctrl.ctrl_key.empty()) {
          server_config_.interface.customized_predict_http.streaming_ctrl.ctrl_key = "streaming";
        }
      }
      if (streaming_ctrl["res_content_type"] && streaming_ctrl["res_content_type"].IsNull()) {
        server_config_.interface.customized_predict_http.streaming_ctrl.res_content_type = "application/octet-stream";
      } else {
        YAML_TRY_EXTRACT(streaming_ctrl, res_content_type, std::string,
                         server_config_.interface.customized_predict_http.streaming_ctrl.res_content_type);
        if (server_config_.interface.customized_predict_http.streaming_ctrl.res_content_type.empty()) {
          server_config_.interface.customized_predict_http.streaming_ctrl.res_content_type = "application/octet-stream";
        }
      }
    }
  }

  YAML_TRY_EXTRACT(server_conf, max_connections, int, server_config_.max_connections);
  server_config_._is_set.max_connections = true;
  YAML_TRY_EXTRACT(server_conf, max_concurrency, int, server_config_.max_concurrency);
  server_config_._is_set.max_concurrency = true;

  auto gpu_conf = server_conf["gpu"];
  if (gpu_conf && !gpu_conf.IsNull() && gpu_conf.IsMap()) {
    YAML_TRY_EXTRACT(gpu_conf, mem_manager_type, std::string, server_config_.gpu.mem_manager_type);
    if (server_config_.gpu.mem_manager_type != "none") {
      YAML_TRY_EXTRACT(gpu_conf, mem_limit_mib, int, server_config_.gpu.mem_lim_mib);
      YAML_TRY_EXTRACT(gpu_conf, mem_gc_enable, bool, server_config_.gpu.mem_gc_enable);
      YAML_TRY_EXTRACT(gpu_conf, mem_gc_interval, int, server_config_.gpu.mem_gc_interval_s);
    }
    auto devices_conf = gpu_conf["devices"];
    if (!devices_conf || devices_conf.IsNull() || !devices_conf.IsSequence()) {
      std::cerr << "[server.yml] Server gpu devices conf is invalid." << std::endl;
      return false;
    }
    for (const auto& device : devices_conf) {
      if (!device || device.IsNull() || !device.IsScalar()) {
        std::cerr << "[server.yml] Server gpu devices conf is invalid." << std::endl;
        return false;
      }
      server_config_.gpu.devices.emplace_back(device.as<int>());
    }
    server_config_._is_set.gpu = true;
  }

  auto log_conf = server_conf["log"];
  if (!log_conf || log_conf.IsNull() || !log_conf.IsMap()) {
    std::cerr << "[server.yml] log conf is invalid." << std::endl;
    return false;
  }
  YAML_TRY_EXTRACT(log_conf, log_dir, std::string, server_config_.log.log_dir);
  YAML_TRY_EXTRACT(log_conf, log_backup_count, int, server_config_.log.log_backup_count);
  if (server_config_.log.log_backup_count < 1) {
    std::cerr << "[server.yml] log_backup_count must not be less than 1." << std::endl;
    return false;
  }
  server_config_._is_set.log = true;

  // std::cout << "Server config: \n" << server_config_.ToString() << std::endl;
  return true;
}

bool GlobalConfig::LoadInferenceConf(const std::string& conf_path) {
  YAML::Node inference_conf = YAML::LoadFile(conf_path);
  if (!inference_conf || inference_conf.IsNull() || !inference_conf.IsMap()) {
    std::cerr << "[inference.yml] Failed to load inference config file: " << conf_path << std::endl;
    return false;
  }

  auto models_conf = inference_conf["models"];
  if (!models_conf || models_conf.IsNull() || !models_conf.IsSequence()) {
    std::cerr << "[inference.yml] Models conf is invalid." << std::endl;
    return false;
  }
  for (const auto& model_conf : models_conf) {
    if (!model_conf || model_conf.IsNull() || !model_conf.IsMap()) {
      std::cerr << "[inference.yml] Model conf is invalid." << std::endl;
      return false;
    }
    InferenceConfig::ModelConfig model_config;
    YAML_TRY_EXTRACT(model_conf, name, std::string, model_config.name);
    YAML_TRY_EXTRACT(model_conf, version, std::string, model_config.version);
    YAML_TRY_EXTRACT(model_conf, device, std::string, model_config.device);
    YAML_TRY_EXTRACT(model_conf, inferer_type, std::string, model_config.inferer_type);
    if (model_config.device == "original" && model_config.inferer_type == "torch") {
      YAML_TRY_EXTRACT(model_conf, inp_device, std::string, model_config.inp_device);
    }
    YAML_TRY_EXTRACT(model_conf, inferer_name, std::string, model_config.inferer_name);
    YAML_TRY_EXTRACT(model_conf, inferer_path, std::string, model_config.inferer_path);
    YAML_TRY_EXTRACT(model_conf, inferer_args, YAML::Node, model_config.inferer_args);
    YAML_TRY_EXTRACT(model_conf, converter_type, std::string, model_config.converter_type);
    YAML_TRY_EXTRACT(model_conf, converter_name, std::string, model_config.converter_name);
    YAML_TRY_EXTRACT(model_conf, converter_path, std::string, model_config.converter_path);
    YAML_TRY_EXTRACT(model_conf, converter_args, YAML::Node, model_config.converter_args);
    auto batching_conf = model_conf["batching"];
    if (batching_conf && !batching_conf.IsNull() && batching_conf.IsMap()) {
      YAML_TRY_EXTRACT(batching_conf, type, std::string, model_config.batching.type);
      YAML_TRY_EXTRACT(batching_conf, max_batch_size, int, model_config.batching.max_batch_size);
      YAML_TRY_EXTRACT(batching_conf, batch_timeout_us, int, model_config.batching.batch_timeout_us);
    } else {
      model_config.batching.type = "none";
    }
    if (inference_config_.models.find(model_config.name + "-" + model_config.version) !=
        inference_config_.models.end()) {
      std::cerr << "[inference.yml] Model " << model_config.name << "-" << model_config.version
                << " has already existed." << std::endl;
      return false;
    }
    inference_config_.models.emplace(model_config.name + "-" + model_config.version, std::move(model_config));
  }
  inference_config_._is_set.models = true;

  auto dag_conf = inference_conf["dag"];
  if (!dag_conf || dag_conf.IsNull() || !dag_conf.IsMap()) {
    std::cerr << "[inference.yml] Dag conf is invalid." << std::endl;
    return false;
  }
  YAML_TRY_EXTRACT(dag_conf, type, std::string, inference_config_.dag.type);
  YAML_TRY_EXTRACT(dag_conf, name, std::string, inference_config_.dag.name);
  auto nodes_conf = dag_conf["nodes"];
  if (!nodes_conf || nodes_conf.IsNull() || !nodes_conf.IsSequence()) {
    std::cerr << "[inference.yml] Nodes conf is invalid." << std::endl;
    return false;
  }
  for (const auto& node : nodes_conf) {
    if (!node || node.IsNull() || !node.IsMap()) {
      std::cerr << "[inference.yml] Node conf is invalid." << std::endl;
      return false;
    }
    InferenceConfig::NodeConfig node_config;
    YAML_TRY_EXTRACT(node, name, std::string, node_config.name);
    YAML_TRY_EXTRACT(node, type, std::string, node_config.type);
    YAML_TRY_EXTRACT(node, model, std::string, node_config.model);
    inference_config_.dag.nodes.emplace_back(std::move(node_config));
  }
  inference_config_._is_set.dag = true;

  // std::cout << "Inference config: \n" << inference_config_.ToString() << std::endl;
  return true;
}
} // namespace netease::grps
