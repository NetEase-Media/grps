/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2024/06/14
 * Brief  TensorRT tensor converters. Converter grps msg to trt tensor when preprocess, and convert trt tensor to grps
 * msg when postprocess.
 */

#pragma once

#include "converter/converter.h"

namespace netease::grps {
class TrtTensorConverter : public Converter {
public:
  // Clone converter for duplicated use. Don't edit this function.
  Converter* Clone() override { return new TrtTensorConverter(); }

  /**
   * @brief Init converter.
   * @param path: Path.
   * @param args: More args.
   * @throw ConverterException: If init failed, throw ConverterException and will be caught by server and show error
   * message to user when start service.
   */
  void Init(const std::string& path, const YAML::Node& args) override;

  /**
   * @brief PreProcess input message.
   * @param input: Input message from client or previous model(multi model sequential mode).
   * @param output: Input tensor of model inferer.
   * @param ctx: Context of current request.
   * @throw ConverterException: If pre-process failed, throw ConverterException and will be caught by server and return
   * error message to client.
   */
  void PreProcess(const ::grps::protos::v1::GrpsMessage& input,
                  std::vector<std::pair<std::string, TensorWrapper>>& output,
                  GrpsContext& ctx) override;

  /**
   * @brief PostProcess output tensor.
   * @param input: Output tensor of model inferer.
   * @param output: Output message to client or next model(multi model sequential mode).
   * @param ctx: Context of current request.
   * @throw ConverterException: If post-process failed, throw ConverterException and will be caught by server and
   * return error message to client.
   */
  void PostProcess(const std::vector<std::pair<std::string, TensorWrapper>>& input,
                   ::grps::protos::v1::GrpsMessage& output,
                   GrpsContext& ctx) override;

  /**
   * @brief Batch pre-process input message.
   * @param inputs: Inputs in batch messages from client or previous model(multi model sequential mode).
   * @param output: Input tensor of model inferer.
   * @param ctxs: Contexts of each request in batch.
   * @throw ConverterException: If pre-process failed, can throw ConverterException and will be caught by server and
   * return error message to client.
   */
  void BatchPreProcess(std::vector<const ::grps::protos::v1::GrpsMessage*>& inputs,
                       std::vector<std::pair<std::string, TensorWrapper>>& output,
                       std::vector<GrpsContext*>& ctxs) override;

  /**
   * @brief Batch post-process output tensor.
   * @param input: Output tensor of model inferer.
   * @param outputs: Output messages in batch to client or next model(multi model sequential mode).
   * @param ctxs: Contexts of each request in batch.
   * @throw ConverterException: If post-process failed, can throw ConverterException and will be caught by server and
   * return error message to client.
   */
  void BatchPostProcess(const std::vector<std::pair<std::string, TensorWrapper>>& input,
                        std::vector<::grps::protos::v1::GrpsMessage*>& outputs,
                        std::vector<GrpsContext*>& ctxs) override;

private:
  static nvinfer1::DataType GTensorType2TrtType(::grps::protos::v1::DataType dtype);

  // Convert generic tensor to trt tensor.
  static void GTensor2TrtTensor(const ::grps::protos::v1::GenericTensor& g_tensor,
                                const std::string& tensor_name,
                                TrtHostBinding& tensor,
                                size_t tensor_size,
                                size_t offset);

  // Convert trt tensor to generic tensor.
  static void TrtTensor2GTensor(TrtHostBinding& tensor,
                                const std::string& tensor_name,
                                ::grps::protos::v1::GenericTensor& g_tensor,
                                size_t tensor_size,
                                size_t offset);
};
} // namespace netease::grps