// Customized deep learning model inferer. Including model load and model infer.

#pragma once

#include "model_infer/inferer.h"

namespace netease::grps {
class YourInferer : public ModelInferer {
public:
  YourInferer();
  ~YourInferer() override;

  // Clone inferer for duplicated use. Don't edit this function.
  ModelInferer* Clone() override { return new YourInferer(); }

  /**
   * @brief Init model inferer.
   * @param path: Model path, it can be a file path or a directory path.
   * @param device: Device to run model.
   * @param args: More args.
   * @throw InfererException: If init failed, can throw InfererException and will be caught by server and show error
   * message to user when start service.
   */
  void Init(const std::string& path, const std::string& device, const YAML::Node& args) override;

  /**
   * @brief Load model.
   * @throw InfererException: If load failed, can throw InfererException and will be caught by server and show error
   * message to user when start service.
   */
  void Load() override;

  /**
   * @brief Infer model.
   * @param inputs: Input tensor of model.
   * @param outputs: Output tensor of model.
   * @param ctx: Context of current request.
   * @throw InfererException: If infer failed, can throw InfererException and will be caught by server and return error
   * message to client.
   */
  void Infer(const std::vector<std::pair<std::string, TensorWrapper>>& inputs,
             std::vector<std::pair<std::string, TensorWrapper>>& outputs,
             GrpsContext& ctx) override;

  /**
   * @brief Infer model in batch.
   * @param inputs: Input tensor of model in batch.
   * @param outputs: Output tensor of model in batch.
   * @param ctxs: Contexts of each request in batch.
   * @throw InfererException: If infer failed, can throw InfererException and will be caught by server and return error
   * message to client.
   */
  void BatchInfer(const std::vector<std::pair<std::string, TensorWrapper>>& inputs,
                  std::vector<std::pair<std::string, TensorWrapper>>& outputs,
                  std::vector<GrpsContext*>& ctxs) override;

  // --------------------------------------- No converter mode [BEGIN] ---------------------------------------

  /**
   * Used when in `no converter mode`. Input and output are directly GrpsMessage.
   * @brief Infer model.
   * @param input: Input.
   * @param output: Output.
   * @param ctx: Context of current request.
   * @throw InfererException: If infer failed, can throw InfererException and will be caught by server and return error
   * message to client.
   */
  void Infer(const ::grps::protos::v1::GrpsMessage& input,
             ::grps::protos::v1::GrpsMessage& output,
             GrpsContext& ctx) override;

  /**
   * Used when in `no converter mode`. Inputs and outputs are directly GrpsMessage vector.
   * @param inputs: Inputs in batch.
   * @param outputs: Outputs in batch.
   * @param ctxs: Contexts of each request in batch.
   * @throw InfererException: If infer failed, can throw InfererException and will be caught by server and return error
   * message to client.
   */
  void BatchInfer(std::vector<const ::grps::protos::v1::GrpsMessage*>& inputs,
                  std::vector<::grps::protos::v1::GrpsMessage*>& outputs,
                  std::vector<GrpsContext*>& ctxs) override;

  // --------------------------------------- No converter mode [END] ---------------------------------------
};

REGISTER_INFERER(YourInferer, your_inferer); // Register your inferer.

// Define other inferer class after here.

} // namespace netease::grps