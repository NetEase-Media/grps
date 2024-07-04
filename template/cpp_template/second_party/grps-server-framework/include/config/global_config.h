/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/08
 * Brief  global configs.
 */

#pragma once

#include <yaml-cpp/yaml.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace netease::grps {
class GlobalConfig {
public:
  // Corresponding to server.yml
  struct ServerConfig {
    struct {
      std::string framework;
      std::string host;
      std::string port;
      struct {
        std::string path;
        bool customized_body = false;
      } customized_predict_http;
    } interface;

    int max_connections = 0;
    int max_concurrency = 0;

    struct GPUConfig {
      std::string mem_manager_type;
      int mem_lim_mib{};
      bool mem_gc_enable{};
      int mem_gc_interval_s{};
      std::vector<int> devices{};
    } gpu;

    struct {
      std::string log_dir;
      int log_backup_count{};
    } log;

    struct {
      bool interface = false;
      bool customized_predict_http = false;
      bool max_connections = false;
      bool max_concurrency = false;
      bool gpu = false;
      bool log = false;
    } _is_set{};

    [[nodiscard]] std::string ToString() const {
      std::stringstream ss;
      if (_is_set.interface) {
        ss << "interface: " << interface.framework << " " << interface.host << " " << interface.port << std::endl;
      }
      if (_is_set.customized_predict_http) {
        ss << "customized_predict_http: " << interface.customized_predict_http.path << " "
           << interface.customized_predict_http.customized_body << std::endl;
      }
      if (_is_set.max_connections) {
        ss << "max_connections: " << max_connections << std::endl;
      }
      if (_is_set.max_concurrency) {
        ss << "max_concurrency: " << max_concurrency << std::endl;
      }
      if (_is_set.gpu) {
        ss << "gpu: " << gpu.mem_manager_type << " " << gpu.mem_lim_mib << " " << gpu.mem_gc_enable << " "
           << gpu.mem_gc_interval_s << std::endl;
      }
      if (_is_set.log) {
        ss << "log: " << log.log_dir << " " << log.log_backup_count << std::endl;
      }
      return ss.str();
    }
  };

  // Corresponding to inference.yml
  struct InferenceConfig {
    struct ModelConfig {
      std::string name;
      std::string version;
      std::string device;
      std::string inp_device;
      std::string inferer_type;
      std::string inferer_name;
      std::string inferer_path;
      YAML::Node inferer_args;
      std::string converter_type;
      std::string converter_name;
      std::string converter_path;
      YAML::Node converter_args;
      struct BatchingConfig {
        std::string type;
        int max_batch_size{};
        int batch_timeout_us{};
      } batching;
    };
    std::unordered_map<std::string, ModelConfig> models;

    struct NodeConfig {
      std::string name;
      std::string type;
      std::string model;
    };

    struct {
      std::string type;
      std::string name;
      std::vector<NodeConfig> nodes;
    } dag;

    struct {
      bool models = false;
      bool dag = false;
    } _is_set{};

    [[nodiscard]] std::string ToString() const {
      std::stringstream ss;
      if (_is_set.models) {
        ss << "models: " << std::endl;
        for (const auto& [model_name, model_config] : models) {
          ss << "  " << model_name << ": " << model_config.name << " " << model_config.version << " "
             << model_config.device << " " << model_config.inp_device << " " << model_config.inferer_type << " "
             << model_config.inferer_name << " " << model_config.inferer_path << " " << model_config.inferer_args
             << model_config.converter_name << " " << model_config.converter_path << " " << model_config.converter_args
             << " " << model_config.batching.type << " " << model_config.batching.max_batch_size << " "
             << model_config.batching.batch_timeout_us << std::endl;
        }
      }
      if (_is_set.dag) {
        ss << "dag: " << dag.type << " " << dag.name << std::endl;
        for (const auto& node : dag.nodes) {
          ss << "  " << node.name << ": " << node.type << " " << node.model << std::endl;
        }
      }
      return ss.str();
    }
  };

  static GlobalConfig& Instance() {
    static GlobalConfig instance;
    return instance;
  }

  ~GlobalConfig() = default;
  GlobalConfig(const GlobalConfig&) = delete;
  GlobalConfig& operator=(const GlobalConfig&) = delete;
  GlobalConfig(GlobalConfig&&) = delete;
  GlobalConfig& operator=(GlobalConfig&&) = delete;

  [[nodiscard]] const ServerConfig& server_config() const { return server_config_; }
  [[nodiscard]] const InferenceConfig& inference_config() const { return inference_config_; }

  bool Load(const std::string& server_conf_path = "./conf/server.yml",
            const std::string& inference_conf_path = "./conf/inference.yml");

private:
  GlobalConfig() = default;

  ServerConfig server_config_;
  InferenceConfig inference_config_;

  bool LoadServerConf(const std::string& conf_path);

  bool LoadInferenceConf(const std::string& conf_path);
}; // class GlobalConfig

} // namespace netease::grps