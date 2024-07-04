/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2024/06/04
 * Brief  TensorRT model inferer implementation.
 */

#pragma once

#include "converter/trt_tensor_converter.h"
#include "model_infer/inferer.h"
#include "model_infer/trt_cuda.h"

namespace netease::grps {
class TrtModelInferer : public ModelInferer {
public:
  class TrtDeviceBinding {
  public:
    /**
     * Constructor.
     * @param name: Binding name.
     * @param dims: Binding dimensions shape.
     * @param data_type: Data type.
     * @param vec_dim: Dimension index that the buffer is vectorized.
     * @param comps: The number of components included in one element.
     * @param is_input_binding: Whether this binding is an input binding.
     * @param is_shape_binding: Whether this binding is a shape binding.
     */
    explicit TrtDeviceBinding(const char* name,
                              const nvinfer1::Dims& dims,
                              const nvinfer1::Dims& max_dims,
                              const nvinfer1::DataType& data_type,
                              int vec_dim = -1,
                              int comps = -1,
                              bool is_input_binding = false,
                              bool is_shape_binding = false)
        : name_(name)
        , dims_(dims)
        , max_dims_(max_dims)
        , data_type_(data_type)
        , vec_dim_(vec_dim)
        , comps_(comps)
        , data_type_size_(TrtDtypeSize(data_type))
        , volume_(0)
        , buffer_size_(0)
        , buffer_capacity_(0)
        , is_input_binding_(is_input_binding)
        , is_shape_binding_(is_shape_binding) {}

    TrtDeviceBinding(const TrtDeviceBinding&) = delete;
    TrtDeviceBinding& operator=(const TrtDeviceBinding&) = delete;
    TrtDeviceBinding(TrtDeviceBinding&&) = default;
    TrtDeviceBinding& operator=(TrtDeviceBinding&&) = default;

    void Allocate(const nvinfer1::Dims& true_dims);

    void FromHost(TrtHostBinding& host_binding, CudaStream& stream);

    void ToHost(CudaStream& stream, TrtHostBinding& host_binding);

    [[nodiscard]] const char* name() const { return name_; }

    [[nodiscard]] const nvinfer1::Dims& dims() const { return dims_; }

    [[nodiscard]] const nvinfer1::Dims& max_dims() const { return max_dims_; }

    [[nodiscard]] const nvinfer1::Dims& true_dims() const { return true_dims_; }

    [[nodiscard]] int vec_dim() const { return vec_dim_; }

    [[nodiscard]] int comps() const { return comps_; }

    [[nodiscard]] nvinfer1::DataType data_type() const { return data_type_; }

    [[nodiscard]] uint8_t data_type_size() const { return data_type_size_; }

    [[nodiscard]] size_t volume() const { return volume_; }

    [[nodiscard]] size_t buffer_size() const { return buffer_size_; }

    [[nodiscard]] size_t buffer_capacity() const { return buffer_capacity_; }

    [[nodiscard]] DeviceBuffer& buffer() { return buffer_; }

    [[nodiscard]] bool is_input_binding() const { return is_input_binding_; }

    [[nodiscard]] bool is_shape_binding() const { return is_shape_binding_; }

    [[nodiscard]] std::string DebugString() {
      std::string str = "TrtDeviceBinding: {";
      str += "name: " + std::string(name_) + ", ";
      str += "dims: [";
      for (int i = 0; i < dims_.nbDims; ++i) {
        str += std::to_string(dims_.d[i]);
        if (i != dims_.nbDims - 1) {
          str += ", ";
        }
      }
      str += "], ";
      str += "max_dims: [";
      for (int i = 0; i < max_dims_.nbDims; ++i) {
        str += std::to_string(max_dims_.d[i]);
        if (i != max_dims_.nbDims - 1) {
          str += ", ";
        }
      }
      str += "], ";
      str += "true_dims: [";
      for (int i = 0; i < true_dims_.nbDims; ++i) {
        str += std::to_string(true_dims_.d[i]);
        if (i != true_dims_.nbDims - 1) {
          str += ", ";
        }
      }
      str += "], ";
      str += "data_type: " + std::to_string(int(data_type_)) + ", ";
      str += "data_type_size: " + std::to_string(data_type_size_) + ", ";
      str += "volume: " + std::to_string(volume_) + ", ";
      str += "buffer_size: " + std::to_string(buffer_size_) + ", ";
      str += "buffer_capacity: " + std::to_string(buffer_capacity_) + ", ";
      str += "is_input_binding: " + std::to_string(is_input_binding_) + ", ";
      str += "is_shape_binding: " + std::to_string(is_shape_binding_);
      str += "}";
      return str;
    }

  private:
    const char* name_;             // Binding name.
    nvinfer1::Dims dims_;          // Binding dimensions shape that may have dynamic dim.
    nvinfer1::Dims max_dims_;      // Binding dimensions shape that dynamic dim is set to max.
    nvinfer1::Dims true_dims_{};   // Binding dimensions shape that dynamic dim is set to actual size.
    nvinfer1::DataType data_type_; // Binding data type.
    int vec_dim_;                  // Dimension index that the buffer is vectorized.
    int comps_;                    // The number of components included in one element.
    uint8_t data_type_size_;       // Size of this data type(bytes).
    size_t volume_;                // Data volume.
    DeviceBuffer buffer_;          // Binding device buffer.
    size_t buffer_size_;           // Buffer size.
    size_t buffer_capacity_;       // Buffer capacity.
    bool is_input_binding_;        // Whether this binding is an input binding.
    bool is_shape_binding_;        // Whether this binding is a shape binding.
  };

  TrtModelInferer();
  ~TrtModelInferer() override;

  // Clone inferer for duplicated use. Don't edit this function.
  ModelInferer* Clone() override { return new TrtModelInferer(); }

  /**
   * @brief Init model inferer.
   * @param path: Model path, it can be a file path or a directory path.
   * @param device: Device to run model.
   * @param args: More args.
   * @throw InfererException: If init failed, throw InfererException and will be caught by server and show error message
   * to user when start service.
   */
  void Init(const std::string& path, const std::string& device, const YAML::Node& args) override;

  /**
   * @brief Load model.
   * @throw InfererException: If load failed, throw InfererException and will be caught by server and show error
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
   * Not support now.
   * @brief Infer model with profiler.
   * @param profiler_path: Profiler path.
   * @param inputs: Input tensor of model.
   * @param outputs: Output tensor of model.
   * @param ctx: Context of current request.
   * @throw InfererException: If infer failed, can throw InfererException and will be caught by server and return error
   * message to client.
   */
  void InferWithProfiler(const std::string& profiler_path,
                         const std::vector<std::pair<std::string, TensorWrapper>>& inputs,
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

  /**
   * Not support now.
   * @brief Infer model in batch with profiler.
   * @param profiler_path: Profiler path.
   * @param inputs: Input tensor of model in batch.
   * @param outputs: Output tensor of model in batch.
   * @param ctxs: Contexts of each request in batch.
   * @throw InfererException: If infer failed, can throw InfererException and will be caught by server and return error
   * message to client.
   */
  void BatchInferWithProfiler(const std::string& profiler_path,
                              const std::vector<std::pair<std::string, TensorWrapper>>& inputs,
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
   * Not support now. Used when in `no converter mode`. Input and output are directly GrpsMessage.
   * @brief Infer model with profiler.
   * @param profiler_path: Profiler path.
   * @param input: Input.
   * @param output: Output.
   * @param ctx: Context of current request.
   * @throw InfererException: If infer failed, can throw InfererException and will be caught by server and return error
   * message to client.
   */
  void InferWithProfiler(const std::string& profiler_path,
                         const ::grps::protos::v1::GrpsMessage& input,
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

  /**
   * Not support now. Used when in `no converter mode`. Inputs and outputs are directly GrpsMessage vector.
   * @param profiler_path: Profiler path.
   * @param inputs: Inputs in batch.
   * @param outputs: Outputs in batch.
   * @param ctxs: Contexts of each request in batch.
   * @throw InfererException: If infer failed, can throw InfererException and will be caught by server and return error
   * message to client.
   */
  void BatchInferWithProfiler(const std::string& profiler_path,
                              std::vector<const ::grps::protos::v1::GrpsMessage*>& inputs,
                              std::vector<::grps::protos::v1::GrpsMessage*>& outputs,
                              std::vector<GrpsContext*>& ctxs) override;

  // --------------------------------------- No converter mode [END] ---------------------------------------

private:
  struct Instance {
    TrtUniquePtr<nvinfer1::ICudaEngine> engine_;
    TrtUniquePtr<nvinfer1::IExecutionContext> trt_context_;
    CudaStream stream_;
    std::vector<TrtDeviceBinding> bindings_;
    MultiTrtEvent multi_event_;
    std::mutex mutex_;
  };

  TrtUniquePtr<nvinfer1::IRuntime> runtime_;
  std::vector<std::unique_ptr<Instance>> instances_;
  int dla_cores_ = -1;
  int streams_ = 1;
  int device_id_ = 0;
  std::atomic<int> cur_stream_ = 0;  // Current stream index.
  TrtTensorConverter trt_converter_; // Used when no converter mode.
};
} // namespace netease::grps