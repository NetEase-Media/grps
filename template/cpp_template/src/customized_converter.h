// Customized converter of model, including pre-process and post-process.

#pragma once

#include <memory>
#include <unordered_map>

#include "converter/converter.h"

namespace netease::grps {
class YourConverter : public Converter {
public:
  YourConverter();
  ~YourConverter() override;

  // Clone inferer for duplicated use. Don't edit this function.
  Converter* Clone() override { return new YourConverter(); }

  /**
   * @brief Init converter.
   * @param path: Path.
   * @param args: More args.
   * @throw ConverterException: If init failed, can throw ConverterException and will be caught by server and show error
   * message to user when start service.
   */
  void Init(const std::string& path, const YAML::Node& args) override;

  /**
   * @brief PreProcess input message.
   * @param input: Input message from client or previous model(multi model sequential mode).
   * @param output: Input tensor of model inferer.
   * @param ctx: Context of current request.
   * @throw ConverterException: If pre-process failed, can throw ConverterException and will be caught by server and
   * return error message to client.
   */
  void PreProcess(const ::grps::protos::v1::GrpsMessage& input,
                  std::vector<std::pair<std::string, TensorWrapper>>& output,
                  GrpsContext& ctx) override;

  /**
   * @brief PostProcess output tensor.
   * @param input: Output tensor of model inferer.
   * @param output: Output message to client or next model(multi model sequential mode).
   * @param ctx: Context of current request.
   * @throw ConverterException: If post-process failed, can throw ConverterException and will be caught by server and
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
};

REGISTER_CONVERTER(YourConverter, your_converter); // Register your converter.

// Define other converters class after here.

} // namespace netease::grps
