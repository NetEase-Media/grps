/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/04/17
 * Brief  Torch tensor converters. Converter grps msg to torch tensor when preprocess, and convert torch tensor to grps
 * msg when postprocess.
 */

#pragma once

#include "converter/converter.h"

namespace netease::grps {
class TorchTensorConverter : public Converter {
public:
  TorchTensorConverter();

  ~TorchTensorConverter() override;

  // Clone converter for duplicated use. Don't edit this function.
  Converter* Clone() override { return new TorchTensorConverter(); }

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
  static c10::ScalarType GTensorType2TorchType(::grps::protos::v1::DataType dtype);

  // Convert generic tensor to torch tensor.
  static void GTensor2TorchTensor(const ::grps::protos::v1::GenericTensor& g_tensor,
                                  const std::string& tensor_name,
                                  at::Tensor& tensor,
                                  size_t tensor_size,
                                  size_t offset);

  // Convert torch tensor to generic tensor.
  static void TorchTensor2GTensor(const at::Tensor& tensor,
                                  const std::string& tensor_name,
                                  ::grps::protos::v1::GenericTensor& g_tensor,
                                  size_t tensor_size,
                                  size_t offset);
};
} // namespace netease::grps