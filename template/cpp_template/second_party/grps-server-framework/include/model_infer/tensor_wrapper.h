/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/08
 * Brief  tensor wrapper.
 */

#pragma once

#include <memory>

#include "grps.pb.h"

#ifdef GRPS_TF_ENABLE
#include <tensorflow/core/framework/tensor.h>
#else
#include <eigen3/unsupported/Eigen/CXX11/Tensor>
#endif

#ifdef GRPS_TORCH_ENABLE
#include <ATen/core/Tensor.h>
#endif

#ifdef GRPS_TRT_ENABLE
#include "model_infer/trt_cuda.h"
#endif

namespace netease::grps {

#ifdef GRPS_TRT_ENABLE
class TrtHostBinding {
public:
  /**
   * Constructor.
   * @param name: Binding name.
   * @param dims: Binding dimensions shape.
   * @param data_type: Data type.
   * @param vec_dim: Dimension index that the buffer is vectorized.
   * @param comps: The number of components included in one element.
   */
  explicit TrtHostBinding(
    const char* name, const nvinfer1::Dims& dims, const nvinfer1::DataType& data_type, int vec_dim = -1, int comps = -1)
      : name_(name), dims_(dims), data_type_(data_type), data_type_size_(TrtDtypeSize(data_type)) {
    volume_ = Volume(dims, vec_dim, comps);
    buffer_size_ = volume_ * data_type_size_;
    buffer_.Allocate(buffer_size_);
  }

  explicit TrtHostBinding(
    const char* name, nvinfer1::Dims&& dims, const nvinfer1::DataType& data_type, int vec_dim = -1, int comps = -1)
      : name_(name), dims_(dims), data_type_(data_type), data_type_size_(TrtDtypeSize(data_type)) {
    volume_ = Volume(dims, vec_dim, comps);
    buffer_size_ = volume_ * data_type_size_;
    buffer_.Allocate(buffer_size_);
  }

  TrtHostBinding(const TrtHostBinding&) = delete;
  TrtHostBinding& operator=(const TrtHostBinding&) = delete;
  TrtHostBinding(TrtHostBinding&&) = default;
  TrtHostBinding& operator=(TrtHostBinding&&) = default;

  [[nodiscard]] const char* name() const { return name_; }

  [[nodiscard]] const nvinfer1::Dims& dims() const { return dims_; }

  [[nodiscard]] nvinfer1::DataType data_type() const { return data_type_; }

  [[nodiscard]] uint8_t data_type_size() const { return data_type_size_; }

  [[nodiscard]] size_t volume() const { return volume_; }

  [[nodiscard]] size_t buffer_size() const { return buffer_size_; }

  [[nodiscard]] HostBuffer& buffer() { return buffer_; }

  [[nodiscard]] std::string DebugString() {
    std::string str = "TrtHostBinding: {";
    str += "name: " + std::string(name_) + ", ";
    str += "dims: [";
    for (int i = 0; i < dims_.nbDims; ++i) {
      str += std::to_string(dims_.d[i]);
      if (i != dims_.nbDims - 1) {
        str += ", ";
      }
    }
    str += "], ";
    str += "data_type: " + std::to_string(int(data_type_)) + ", ";
    str += "data_type_size: " + std::to_string(data_type_size_) + ", ";
    str += "volume: " + std::to_string(volume_) + ", ";
    str += "buffer_size: " + std::to_string(buffer_size_);
    str += "}";
    return str;
  }

private:
  const char* name_;             // Binding name.
  nvinfer1::Dims dims_;          // Binding dimensions shape.
  nvinfer1::DataType data_type_; // Binding data type.
  uint8_t data_type_size_;       // Size of this data type(bytes).
  size_t volume_;                // Data volume.
  HostBuffer buffer_;            // Binding host buffer.
  size_t buffer_size_;           // Buffer size.
};
#endif

class TensorWrapper {
public:
#ifdef GRPS_TF_ENABLE
  std::shared_ptr<tensorflow::Tensor> tf_tensor;
#endif
#ifdef GRPS_TORCH_ENABLE
  std::shared_ptr<at::Tensor> torch_tensor;
#endif
#ifdef GRPS_TRT_ENABLE
  std::shared_ptr<TrtHostBinding> trt_host_binding;
#endif

  std::shared_ptr<::grps::protos::v1::GenericTensor> generic_tensor;

  // Eigen tensor.
  std::shared_ptr<Eigen::Tensor<float, 1>> eigen_1d_f_tensor;
  std::shared_ptr<Eigen::Tensor<float, 2>> eigen_2d_f_tensor;
  std::shared_ptr<Eigen::Tensor<float, 3>> eigen_3d_f_tensor;
  std::shared_ptr<Eigen::Tensor<float, 4>> eigen_4d_f_tensor;
  std::shared_ptr<Eigen::Tensor<double, 1>> eigen_1d_d_tensor;
  std::shared_ptr<Eigen::Tensor<double, 2>> eigen_2d_d_tensor;
  std::shared_ptr<Eigen::Tensor<double, 3>> eigen_3d_d_tensor;
  std::shared_ptr<Eigen::Tensor<double, 4>> eigen_4d_d_tensor;
  std::shared_ptr<Eigen::Tensor<int, 1>> eigen_1d_i_tensor;
  std::shared_ptr<Eigen::Tensor<int, 2>> eigen_2d_i_tensor;
  std::shared_ptr<Eigen::Tensor<int, 3>> eigen_3d_i_tensor;
  std::shared_ptr<Eigen::Tensor<int, 4>> eigen_4d_i_tensor;
  std::shared_ptr<Eigen::Tensor<int64_t, 1>> eigen_1d_l_tensor;
  std::shared_ptr<Eigen::Tensor<int64_t, 2>> eigen_2d_l_tensor;
  std::shared_ptr<Eigen::Tensor<int64_t, 3>> eigen_3d_l_tensor;
  std::shared_ptr<Eigen::Tensor<int64_t, 4>> eigen_4d_l_tensor;
  std::shared_ptr<Eigen::Tensor<std::string, 1>> eigen_1d_s_tensor;
  std::shared_ptr<Eigen::Tensor<std::string, 2>> eigen_2d_s_tensor;
  std::shared_ptr<Eigen::Tensor<std::string, 3>> eigen_3d_s_tensor;
  std::shared_ptr<Eigen::Tensor<std::string, 4>> eigen_4d_s_tensor;

  TensorWrapper() = default;
  ~TensorWrapper() = default;
  TensorWrapper(const TensorWrapper&) = default;
  TensorWrapper(TensorWrapper&&) = default;
  TensorWrapper& operator=(const TensorWrapper&) = default;
  TensorWrapper& operator=(TensorWrapper&&) = default;

#ifdef GRPS_TF_ENABLE
  // Set tensorflow tensor.
  explicit TensorWrapper(const std::shared_ptr<tensorflow::Tensor>& tf_tensor) : tf_tensor(tf_tensor) {}
  explicit TensorWrapper(std::shared_ptr<tensorflow::Tensor>&& tf_tensor) : tf_tensor(std::move(tf_tensor)) {}
  explicit TensorWrapper(const tensorflow::Tensor& tf_tensor)
      : tf_tensor(std::make_shared<tensorflow::Tensor>(tf_tensor)) {}
  explicit TensorWrapper(tensorflow::Tensor&& tf_tensor)
      : tf_tensor(std::make_shared<tensorflow::Tensor>(std::move(tf_tensor))) {}
#endif

#ifdef GRPS_TORCH_ENABLE
  // Set torch tensor.
  explicit TensorWrapper(const std::shared_ptr<at::Tensor>& torch_tensor) : torch_tensor(torch_tensor) {}
  explicit TensorWrapper(std::shared_ptr<at::Tensor>&& torch_tensor) : torch_tensor(std::move(torch_tensor)) {}
  explicit TensorWrapper(const at::Tensor& torch_tensor) : torch_tensor(std::make_shared<at::Tensor>(torch_tensor)) {}
  explicit TensorWrapper(at::Tensor&& torch_tensor)
      : torch_tensor(std::make_shared<at::Tensor>(std::move(torch_tensor))) {}
#endif

#ifdef GRPS_TRT_ENABLE
  // Set trt host binding.
  explicit TensorWrapper(const std::shared_ptr<TrtHostBinding>& trt_host_binding)
      : trt_host_binding(trt_host_binding) {}
  explicit TensorWrapper(std::shared_ptr<TrtHostBinding>&& trt_host_binding)
      : trt_host_binding(std::move(trt_host_binding)) {}
  explicit TensorWrapper(TrtHostBinding&& trt_host_binding)
      : trt_host_binding(std::make_shared<TrtHostBinding>(std::move(trt_host_binding))) {}
#endif

  // Set generic tensor.
  explicit TensorWrapper(const ::grps::protos::v1::GenericTensor& generic_tensor)
      : generic_tensor(std::make_shared<::grps::protos::v1::GenericTensor>(generic_tensor)) {}
  explicit TensorWrapper(::grps::protos::v1::GenericTensor&& generic_tensor)
      : generic_tensor(std::make_shared<::grps::protos::v1::GenericTensor>(std::move(generic_tensor))) {}
  explicit TensorWrapper(const std::shared_ptr<::grps::protos::v1::GenericTensor>& generic_tensor)
      : generic_tensor(generic_tensor) {}
  explicit TensorWrapper(std::shared_ptr<::grps::protos::v1::GenericTensor>&& generic_tensor)
      : generic_tensor(std::move(generic_tensor)) {}

  // Set eigen tensor.
  explicit TensorWrapper(const std::shared_ptr<Eigen::Tensor<float, 1>>& eigen_1d_f_tensor)
      : eigen_1d_f_tensor(eigen_1d_f_tensor) {}
  explicit TensorWrapper(std::shared_ptr<Eigen::Tensor<float, 1>>&& eigen_1d_f_tensor)
      : eigen_1d_f_tensor(std::move(eigen_1d_f_tensor)) {}
  explicit TensorWrapper(const Eigen::Tensor<float, 1>& eigen_1d_f_tensor)
      : eigen_1d_f_tensor(std::make_shared<Eigen::Tensor<float, 1>>(eigen_1d_f_tensor)) {}
  explicit TensorWrapper(Eigen::Tensor<float, 1>&& eigen_1d_f_tensor)
      : eigen_1d_f_tensor(std::make_shared<Eigen::Tensor<float, 1>>(std::move(eigen_1d_f_tensor))) {}
  explicit TensorWrapper(const std::shared_ptr<Eigen::Tensor<float, 2>>& eigen_2d_f_tensor)
      : eigen_2d_f_tensor(eigen_2d_f_tensor) {}
  explicit TensorWrapper(std::shared_ptr<Eigen::Tensor<float, 2>>&& eigen_2d_f_tensor)
      : eigen_2d_f_tensor(std::move(eigen_2d_f_tensor)) {}
  explicit TensorWrapper(const Eigen::Tensor<float, 2>& eigen_2d_f_tensor)
      : eigen_2d_f_tensor(std::make_shared<Eigen::Tensor<float, 2>>(eigen_2d_f_tensor)) {}
  explicit TensorWrapper(Eigen::Tensor<float, 2>&& eigen_2d_f_tensor)
      : eigen_2d_f_tensor(std::make_shared<Eigen::Tensor<float, 2>>(std::move(eigen_2d_f_tensor))) {}
  explicit TensorWrapper(const std::shared_ptr<Eigen::Tensor<float, 3>>& eigen_3d_f_tensor)
      : eigen_3d_f_tensor(eigen_3d_f_tensor) {}
  explicit TensorWrapper(std::shared_ptr<Eigen::Tensor<float, 3>>&& eigen_3d_f_tensor)
      : eigen_3d_f_tensor(std::move(eigen_3d_f_tensor)) {}
  explicit TensorWrapper(const Eigen::Tensor<float, 3>& eigen_3d_f_tensor)
      : eigen_3d_f_tensor(std::make_shared<Eigen::Tensor<float, 3>>(eigen_3d_f_tensor)) {}
  explicit TensorWrapper(Eigen::Tensor<float, 3>&& eigen_3d_f_tensor)
      : eigen_3d_f_tensor(std::make_shared<Eigen::Tensor<float, 3>>(std::move(eigen_3d_f_tensor))) {}
  explicit TensorWrapper(const std::shared_ptr<Eigen::Tensor<float, 4>>& eigen_4d_f_tensor)
      : eigen_4d_f_tensor(eigen_4d_f_tensor) {}
  explicit TensorWrapper(std::shared_ptr<Eigen::Tensor<float, 4>>&& eigen_4d_f_tensor)
      : eigen_4d_f_tensor(std::move(eigen_4d_f_tensor)) {}
  explicit TensorWrapper(const Eigen::Tensor<float, 4>& eigen_4d_f_tensor)
      : eigen_4d_f_tensor(std::make_shared<Eigen::Tensor<float, 4>>(eigen_4d_f_tensor)) {}
  explicit TensorWrapper(Eigen::Tensor<float, 4>&& eigen_4d_f_tensor)
      : eigen_4d_f_tensor(std::make_shared<Eigen::Tensor<float, 4>>(std::move(eigen_4d_f_tensor))) {}
  explicit TensorWrapper(const std::shared_ptr<Eigen::Tensor<double, 1>>& eigen_1d_d_tensor)
      : eigen_1d_d_tensor(eigen_1d_d_tensor) {}
  explicit TensorWrapper(std::shared_ptr<Eigen::Tensor<double, 1>>&& eigen_1d_d_tensor)
      : eigen_1d_d_tensor(std::move(eigen_1d_d_tensor)) {}
  explicit TensorWrapper(const Eigen::Tensor<double, 1>& eigen_1d_d_tensor)
      : eigen_1d_d_tensor(std::make_shared<Eigen::Tensor<double, 1>>(eigen_1d_d_tensor)) {}
  explicit TensorWrapper(Eigen::Tensor<double, 1>&& eigen_1d_d_tensor)
      : eigen_1d_d_tensor(std::make_shared<Eigen::Tensor<double, 1>>(std::move(eigen_1d_d_tensor))) {}
  explicit TensorWrapper(const std::shared_ptr<Eigen::Tensor<double, 2>>& eigen_2d_d_tensor)
      : eigen_2d_d_tensor(eigen_2d_d_tensor) {}
  explicit TensorWrapper(std::shared_ptr<Eigen::Tensor<double, 2>>&& eigen_2d_d_tensor)
      : eigen_2d_d_tensor(std::move(eigen_2d_d_tensor)) {}
  explicit TensorWrapper(const Eigen::Tensor<double, 2>& eigen_2d_d_tensor)
      : eigen_2d_d_tensor(std::make_shared<Eigen::Tensor<double, 2>>(eigen_2d_d_tensor)) {}
  explicit TensorWrapper(Eigen::Tensor<double, 2>&& eigen_2d_d_tensor)
      : eigen_2d_d_tensor(std::make_shared<Eigen::Tensor<double, 2>>(std::move(eigen_2d_d_tensor))) {}
  explicit TensorWrapper(const std::shared_ptr<Eigen::Tensor<double, 3>>& eigen_3d_d_tensor)
      : eigen_3d_d_tensor(eigen_3d_d_tensor) {}
  explicit TensorWrapper(std::shared_ptr<Eigen::Tensor<double, 3>>&& eigen_3d_d_tensor)
      : eigen_3d_d_tensor(std::move(eigen_3d_d_tensor)) {}
  explicit TensorWrapper(const Eigen::Tensor<double, 3>& eigen_3d_d_tensor)
      : eigen_3d_d_tensor(std::make_shared<Eigen::Tensor<double, 3>>(eigen_3d_d_tensor)) {}
  explicit TensorWrapper(Eigen::Tensor<double, 3>&& eigen_3d_d_tensor)
      : eigen_3d_d_tensor(std::make_shared<Eigen::Tensor<double, 3>>(std::move(eigen_3d_d_tensor))) {}
  explicit TensorWrapper(const std::shared_ptr<Eigen::Tensor<double, 4>>& eigen_4d_d_tensor)
      : eigen_4d_d_tensor(eigen_4d_d_tensor) {}
  explicit TensorWrapper(std::shared_ptr<Eigen::Tensor<double, 4>>&& eigen_4d_d_tensor)
      : eigen_4d_d_tensor(std::move(eigen_4d_d_tensor)) {}
  explicit TensorWrapper(const Eigen::Tensor<double, 4>& eigen_4d_d_tensor)
      : eigen_4d_d_tensor(std::make_shared<Eigen::Tensor<double, 4>>(eigen_4d_d_tensor)) {}
  explicit TensorWrapper(Eigen::Tensor<double, 4>&& eigen_4d_d_tensor)
      : eigen_4d_d_tensor(std::make_shared<Eigen::Tensor<double, 4>>(std::move(eigen_4d_d_tensor))) {}
  explicit TensorWrapper(const std::shared_ptr<Eigen::Tensor<int, 1>>& eigen_1d_i_tensor)
      : eigen_1d_i_tensor(eigen_1d_i_tensor) {}
  explicit TensorWrapper(std::shared_ptr<Eigen::Tensor<int, 1>>&& eigen_1d_i_tensor)
      : eigen_1d_i_tensor(std::move(eigen_1d_i_tensor)) {}
  explicit TensorWrapper(const Eigen::Tensor<int, 1>& eigen_1d_i_tensor)
      : eigen_1d_i_tensor(std::make_shared<Eigen::Tensor<int, 1>>(eigen_1d_i_tensor)) {}
  explicit TensorWrapper(Eigen::Tensor<int, 1>&& eigen_1d_i_tensor)
      : eigen_1d_i_tensor(std::make_shared<Eigen::Tensor<int, 1>>(std::move(eigen_1d_i_tensor))) {}
  explicit TensorWrapper(const std::shared_ptr<Eigen::Tensor<int, 2>>& eigen_2d_i_tensor)
      : eigen_2d_i_tensor(eigen_2d_i_tensor) {}
  explicit TensorWrapper(std::shared_ptr<Eigen::Tensor<int, 2>>&& eigen_2d_i_tensor)
      : eigen_2d_i_tensor(std::move(eigen_2d_i_tensor)) {}
  explicit TensorWrapper(const Eigen::Tensor<int, 2>& eigen_2d_i_tensor)
      : eigen_2d_i_tensor(std::make_shared<Eigen::Tensor<int, 2>>(eigen_2d_i_tensor)) {}
  explicit TensorWrapper(Eigen::Tensor<int, 2>&& eigen_2d_i_tensor)
      : eigen_2d_i_tensor(std::make_shared<Eigen::Tensor<int, 2>>(std::move(eigen_2d_i_tensor))) {}
  explicit TensorWrapper(const std::shared_ptr<Eigen::Tensor<int, 3>>& eigen_3d_i_tensor)
      : eigen_3d_i_tensor(eigen_3d_i_tensor) {}
  explicit TensorWrapper(std::shared_ptr<Eigen::Tensor<int, 3>>&& eigen_3d_i_tensor)
      : eigen_3d_i_tensor(std::move(eigen_3d_i_tensor)) {}
  explicit TensorWrapper(const Eigen::Tensor<int, 3>& eigen_3d_i_tensor)
      : eigen_3d_i_tensor(std::make_shared<Eigen::Tensor<int, 3>>(eigen_3d_i_tensor)) {}
  explicit TensorWrapper(Eigen::Tensor<int, 3>&& eigen_3d_i_tensor)
      : eigen_3d_i_tensor(std::make_shared<Eigen::Tensor<int, 3>>(std::move(eigen_3d_i_tensor))) {}
  explicit TensorWrapper(const std::shared_ptr<Eigen::Tensor<int, 4>>& eigen_4d_i_tensor)
      : eigen_4d_i_tensor(eigen_4d_i_tensor) {}
  explicit TensorWrapper(std::shared_ptr<Eigen::Tensor<int, 4>>&& eigen_4d_i_tensor)
      : eigen_4d_i_tensor(std::move(eigen_4d_i_tensor)) {}
  explicit TensorWrapper(const Eigen::Tensor<int, 4>& eigen_4d_i_tensor)
      : eigen_4d_i_tensor(std::make_shared<Eigen::Tensor<int, 4>>(eigen_4d_i_tensor)) {}
  explicit TensorWrapper(Eigen::Tensor<int, 4>&& eigen_4d_i_tensor)
      : eigen_4d_i_tensor(std::make_shared<Eigen::Tensor<int, 4>>(std::move(eigen_4d_i_tensor))) {}
  explicit TensorWrapper(const std::shared_ptr<Eigen::Tensor<int64_t, 1>>& eigen_1d_l_tensor)
      : eigen_1d_l_tensor(eigen_1d_l_tensor) {}
  explicit TensorWrapper(std::shared_ptr<Eigen::Tensor<int64_t, 1>>&& eigen_1d_l_tensor)
      : eigen_1d_l_tensor(std::move(eigen_1d_l_tensor)) {}
  explicit TensorWrapper(const Eigen::Tensor<int64_t, 1>& eigen_1d_l_tensor)
      : eigen_1d_l_tensor(std::make_shared<Eigen::Tensor<int64_t, 1>>(eigen_1d_l_tensor)) {}
  explicit TensorWrapper(Eigen::Tensor<int64_t, 1>&& eigen_1d_l_tensor)
      : eigen_1d_l_tensor(std::make_shared<Eigen::Tensor<int64_t, 1>>(std::move(eigen_1d_l_tensor))) {}
  explicit TensorWrapper(const std::shared_ptr<Eigen::Tensor<int64_t, 2>>& eigen_2d_l_tensor)
      : eigen_2d_l_tensor(eigen_2d_l_tensor) {}
  explicit TensorWrapper(std::shared_ptr<Eigen::Tensor<int64_t, 2>>&& eigen_2d_l_tensor)
      : eigen_2d_l_tensor(std::move(eigen_2d_l_tensor)) {}
  explicit TensorWrapper(const Eigen::Tensor<int64_t, 2>& eigen_2d_l_tensor)
      : eigen_2d_l_tensor(std::make_shared<Eigen::Tensor<int64_t, 2>>(eigen_2d_l_tensor)) {}
  explicit TensorWrapper(Eigen::Tensor<int64_t, 2>&& eigen_2d_l_tensor)
      : eigen_2d_l_tensor(std::make_shared<Eigen::Tensor<int64_t, 2>>(std::move(eigen_2d_l_tensor))) {}
  explicit TensorWrapper(const std::shared_ptr<Eigen::Tensor<int64_t, 3>>& eigen_3d_l_tensor)
      : eigen_3d_l_tensor(eigen_3d_l_tensor) {}
  explicit TensorWrapper(std::shared_ptr<Eigen::Tensor<int64_t, 3>>&& eigen_3d_l_tensor)
      : eigen_3d_l_tensor(std::move(eigen_3d_l_tensor)) {}
  explicit TensorWrapper(const Eigen::Tensor<int64_t, 3>& eigen_3d_l_tensor)
      : eigen_3d_l_tensor(std::make_shared<Eigen::Tensor<int64_t, 3>>(eigen_3d_l_tensor)) {}
  explicit TensorWrapper(Eigen::Tensor<int64_t, 3>&& eigen_3d_l_tensor)
      : eigen_3d_l_tensor(std::make_shared<Eigen::Tensor<int64_t, 3>>(std::move(eigen_3d_l_tensor))) {}
  explicit TensorWrapper(const std::shared_ptr<Eigen::Tensor<int64_t, 4>>& eigen_4d_l_tensor)
      : eigen_4d_l_tensor(eigen_4d_l_tensor) {}
  explicit TensorWrapper(std::shared_ptr<Eigen::Tensor<int64_t, 4>>&& eigen_4d_l_tensor)
      : eigen_4d_l_tensor(std::move(eigen_4d_l_tensor)) {}
  explicit TensorWrapper(const Eigen::Tensor<int64_t, 4>& eigen_4d_l_tensor)
      : eigen_4d_l_tensor(std::make_shared<Eigen::Tensor<int64_t, 4>>(eigen_4d_l_tensor)) {}
  explicit TensorWrapper(Eigen::Tensor<int64_t, 4>&& eigen_4d_l_tensor)
      : eigen_4d_l_tensor(std::make_shared<Eigen::Tensor<int64_t, 4>>(std::move(eigen_4d_l_tensor))) {}
  explicit TensorWrapper(const std::shared_ptr<Eigen::Tensor<std::string, 1>>& eigen_1d_s_tensor)
      : eigen_1d_s_tensor(eigen_1d_s_tensor) {}
  explicit TensorWrapper(std::shared_ptr<Eigen::Tensor<std::string, 1>>&& eigen_1d_s_tensor)
      : eigen_1d_s_tensor(std::move(eigen_1d_s_tensor)) {}
  explicit TensorWrapper(const Eigen::Tensor<std::string, 1>& eigen_1d_s_tensor)
      : eigen_1d_s_tensor(std::make_shared<Eigen::Tensor<std::string, 1>>(eigen_1d_s_tensor)) {}
  explicit TensorWrapper(Eigen::Tensor<std::string, 1>&& eigen_1d_s_tensor)
      : eigen_1d_s_tensor(std::make_shared<Eigen::Tensor<std::string, 1>>(std::move(eigen_1d_s_tensor))) {}
  explicit TensorWrapper(const std::shared_ptr<Eigen::Tensor<std::string, 2>>& eigen_2d_s_tensor)
      : eigen_2d_s_tensor(eigen_2d_s_tensor) {}
  explicit TensorWrapper(std::shared_ptr<Eigen::Tensor<std::string, 2>>&& eigen_2d_s_tensor)
      : eigen_2d_s_tensor(std::move(eigen_2d_s_tensor)) {}
  explicit TensorWrapper(const Eigen::Tensor<std::string, 2>& eigen_2d_s_tensor)
      : eigen_2d_s_tensor(std::make_shared<Eigen::Tensor<std::string, 2>>(eigen_2d_s_tensor)) {}
  explicit TensorWrapper(Eigen::Tensor<std::string, 2>&& eigen_2d_s_tensor)
      : eigen_2d_s_tensor(std::make_shared<Eigen::Tensor<std::string, 2>>(std::move(eigen_2d_s_tensor))) {}
  explicit TensorWrapper(const std::shared_ptr<Eigen::Tensor<std::string, 3>>& eigen_3d_s_tensor)
      : eigen_3d_s_tensor(eigen_3d_s_tensor) {}
  explicit TensorWrapper(std::shared_ptr<Eigen::Tensor<std::string, 3>>&& eigen_3d_s_tensor)
      : eigen_3d_s_tensor(std::move(eigen_3d_s_tensor)) {}
  explicit TensorWrapper(const Eigen::Tensor<std::string, 3>& eigen_3d_s_tensor)
      : eigen_3d_s_tensor(std::make_shared<Eigen::Tensor<std::string, 3>>(eigen_3d_s_tensor)) {}
  explicit TensorWrapper(Eigen::Tensor<std::string, 3>&& eigen_3d_s_tensor)
      : eigen_3d_s_tensor(std::make_shared<Eigen::Tensor<std::string, 3>>(std::move(eigen_3d_s_tensor))) {}
  explicit TensorWrapper(const std::shared_ptr<Eigen::Tensor<std::string, 4>>& eigen_4d_s_tensor)
      : eigen_4d_s_tensor(eigen_4d_s_tensor) {}
  explicit TensorWrapper(std::shared_ptr<Eigen::Tensor<std::string, 4>>&& eigen_4d_s_tensor)
      : eigen_4d_s_tensor(std::move(eigen_4d_s_tensor)) {}
  explicit TensorWrapper(const Eigen::Tensor<std::string, 4>& eigen_4d_s_tensor)
      : eigen_4d_s_tensor(std::make_shared<Eigen::Tensor<std::string, 4>>(eigen_4d_s_tensor)) {}
  explicit TensorWrapper(Eigen::Tensor<std::string, 4>&& eigen_4d_s_tensor)
      : eigen_4d_s_tensor(std::make_shared<Eigen::Tensor<std::string, 4>>(std::move(eigen_4d_s_tensor))) {}
};
} // namespace netease::grps