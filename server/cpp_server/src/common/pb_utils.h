/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/02/06
 * Brief  protobuf utils.
 */

#pragma once

#include <google/protobuf/util/json_util.h>
#include <json2pb/json_to_pb.h>

namespace netease::grps {
static inline std::string Pb2json(const ::google::protobuf::Message& message, std::string* err = nullptr) {
  std::string json;
  google::protobuf::util::JsonPrintOptions options;
  options.always_print_enums_as_ints = false;
  options.add_whitespace = true;
  options.preserve_proto_field_names = true;
  options.always_print_primitive_fields = false;
  google::protobuf::util::MessageToJsonString(message, &json, options);
  return json;
}

static inline void Json2pb(const std::string& json, ::google::protobuf::Message* message, std::string* err = nullptr) {
  json2pb::Json2PbOptions options;
  options.base64_to_bytes = true;
  json2pb::JsonToProtoMessage(json, message, options, err);
}
} // namespace netease::grps