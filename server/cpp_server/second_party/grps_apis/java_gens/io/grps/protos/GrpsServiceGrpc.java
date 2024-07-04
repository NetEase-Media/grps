package io.grps.protos;

import static io.grpc.MethodDescriptor.generateFullMethodName;

/**
 */
@javax.annotation.Generated(
    value = "by gRPC proto compiler (version 1.46.0)",
    comments = "Source: grps.proto")
@io.grpc.stub.annotations.GrpcGenerated
public final class GrpsServiceGrpc {

  private GrpsServiceGrpc() {}

  public static final String SERVICE_NAME = "grps.protos.v1.GrpsService";

  // Static method descriptors that strictly reflect the proto.
  private static volatile io.grpc.MethodDescriptor<io.grps.protos.GrpsProtos.GrpsMessage,
      io.grps.protos.GrpsProtos.GrpsMessage> getPredictMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "Predict",
      requestType = io.grps.protos.GrpsProtos.GrpsMessage.class,
      responseType = io.grps.protos.GrpsProtos.GrpsMessage.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<io.grps.protos.GrpsProtos.GrpsMessage,
      io.grps.protos.GrpsProtos.GrpsMessage> getPredictMethod() {
    io.grpc.MethodDescriptor<io.grps.protos.GrpsProtos.GrpsMessage, io.grps.protos.GrpsProtos.GrpsMessage> getPredictMethod;
    if ((getPredictMethod = GrpsServiceGrpc.getPredictMethod) == null) {
      synchronized (GrpsServiceGrpc.class) {
        if ((getPredictMethod = GrpsServiceGrpc.getPredictMethod) == null) {
          GrpsServiceGrpc.getPredictMethod = getPredictMethod =
              io.grpc.MethodDescriptor.<io.grps.protos.GrpsProtos.GrpsMessage, io.grps.protos.GrpsProtos.GrpsMessage>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "Predict"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  io.grps.protos.GrpsProtos.GrpsMessage.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  io.grps.protos.GrpsProtos.GrpsMessage.getDefaultInstance()))
              .setSchemaDescriptor(new GrpsServiceMethodDescriptorSupplier("Predict"))
              .build();
        }
      }
    }
    return getPredictMethod;
  }

  private static volatile io.grpc.MethodDescriptor<io.grps.protos.GrpsProtos.GrpsMessage,
      io.grps.protos.GrpsProtos.GrpsMessage> getPredictStreamingMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "PredictStreaming",
      requestType = io.grps.protos.GrpsProtos.GrpsMessage.class,
      responseType = io.grps.protos.GrpsProtos.GrpsMessage.class,
      methodType = io.grpc.MethodDescriptor.MethodType.SERVER_STREAMING)
  public static io.grpc.MethodDescriptor<io.grps.protos.GrpsProtos.GrpsMessage,
      io.grps.protos.GrpsProtos.GrpsMessage> getPredictStreamingMethod() {
    io.grpc.MethodDescriptor<io.grps.protos.GrpsProtos.GrpsMessage, io.grps.protos.GrpsProtos.GrpsMessage> getPredictStreamingMethod;
    if ((getPredictStreamingMethod = GrpsServiceGrpc.getPredictStreamingMethod) == null) {
      synchronized (GrpsServiceGrpc.class) {
        if ((getPredictStreamingMethod = GrpsServiceGrpc.getPredictStreamingMethod) == null) {
          GrpsServiceGrpc.getPredictStreamingMethod = getPredictStreamingMethod =
              io.grpc.MethodDescriptor.<io.grps.protos.GrpsProtos.GrpsMessage, io.grps.protos.GrpsProtos.GrpsMessage>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.SERVER_STREAMING)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "PredictStreaming"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  io.grps.protos.GrpsProtos.GrpsMessage.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  io.grps.protos.GrpsProtos.GrpsMessage.getDefaultInstance()))
              .setSchemaDescriptor(new GrpsServiceMethodDescriptorSupplier("PredictStreaming"))
              .build();
        }
      }
    }
    return getPredictStreamingMethod;
  }

  private static volatile io.grpc.MethodDescriptor<io.grps.protos.GrpsProtos.GrpsMessage,
      io.grps.protos.GrpsProtos.GrpsMessage> getOnlineMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "Online",
      requestType = io.grps.protos.GrpsProtos.GrpsMessage.class,
      responseType = io.grps.protos.GrpsProtos.GrpsMessage.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<io.grps.protos.GrpsProtos.GrpsMessage,
      io.grps.protos.GrpsProtos.GrpsMessage> getOnlineMethod() {
    io.grpc.MethodDescriptor<io.grps.protos.GrpsProtos.GrpsMessage, io.grps.protos.GrpsProtos.GrpsMessage> getOnlineMethod;
    if ((getOnlineMethod = GrpsServiceGrpc.getOnlineMethod) == null) {
      synchronized (GrpsServiceGrpc.class) {
        if ((getOnlineMethod = GrpsServiceGrpc.getOnlineMethod) == null) {
          GrpsServiceGrpc.getOnlineMethod = getOnlineMethod =
              io.grpc.MethodDescriptor.<io.grps.protos.GrpsProtos.GrpsMessage, io.grps.protos.GrpsProtos.GrpsMessage>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "Online"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  io.grps.protos.GrpsProtos.GrpsMessage.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  io.grps.protos.GrpsProtos.GrpsMessage.getDefaultInstance()))
              .setSchemaDescriptor(new GrpsServiceMethodDescriptorSupplier("Online"))
              .build();
        }
      }
    }
    return getOnlineMethod;
  }

  private static volatile io.grpc.MethodDescriptor<io.grps.protos.GrpsProtos.GrpsMessage,
      io.grps.protos.GrpsProtos.GrpsMessage> getOfflineMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "Offline",
      requestType = io.grps.protos.GrpsProtos.GrpsMessage.class,
      responseType = io.grps.protos.GrpsProtos.GrpsMessage.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<io.grps.protos.GrpsProtos.GrpsMessage,
      io.grps.protos.GrpsProtos.GrpsMessage> getOfflineMethod() {
    io.grpc.MethodDescriptor<io.grps.protos.GrpsProtos.GrpsMessage, io.grps.protos.GrpsProtos.GrpsMessage> getOfflineMethod;
    if ((getOfflineMethod = GrpsServiceGrpc.getOfflineMethod) == null) {
      synchronized (GrpsServiceGrpc.class) {
        if ((getOfflineMethod = GrpsServiceGrpc.getOfflineMethod) == null) {
          GrpsServiceGrpc.getOfflineMethod = getOfflineMethod =
              io.grpc.MethodDescriptor.<io.grps.protos.GrpsProtos.GrpsMessage, io.grps.protos.GrpsProtos.GrpsMessage>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "Offline"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  io.grps.protos.GrpsProtos.GrpsMessage.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  io.grps.protos.GrpsProtos.GrpsMessage.getDefaultInstance()))
              .setSchemaDescriptor(new GrpsServiceMethodDescriptorSupplier("Offline"))
              .build();
        }
      }
    }
    return getOfflineMethod;
  }

  private static volatile io.grpc.MethodDescriptor<io.grps.protos.GrpsProtos.GrpsMessage,
      io.grps.protos.GrpsProtos.GrpsMessage> getCheckLivenessMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "CheckLiveness",
      requestType = io.grps.protos.GrpsProtos.GrpsMessage.class,
      responseType = io.grps.protos.GrpsProtos.GrpsMessage.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<io.grps.protos.GrpsProtos.GrpsMessage,
      io.grps.protos.GrpsProtos.GrpsMessage> getCheckLivenessMethod() {
    io.grpc.MethodDescriptor<io.grps.protos.GrpsProtos.GrpsMessage, io.grps.protos.GrpsProtos.GrpsMessage> getCheckLivenessMethod;
    if ((getCheckLivenessMethod = GrpsServiceGrpc.getCheckLivenessMethod) == null) {
      synchronized (GrpsServiceGrpc.class) {
        if ((getCheckLivenessMethod = GrpsServiceGrpc.getCheckLivenessMethod) == null) {
          GrpsServiceGrpc.getCheckLivenessMethod = getCheckLivenessMethod =
              io.grpc.MethodDescriptor.<io.grps.protos.GrpsProtos.GrpsMessage, io.grps.protos.GrpsProtos.GrpsMessage>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "CheckLiveness"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  io.grps.protos.GrpsProtos.GrpsMessage.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  io.grps.protos.GrpsProtos.GrpsMessage.getDefaultInstance()))
              .setSchemaDescriptor(new GrpsServiceMethodDescriptorSupplier("CheckLiveness"))
              .build();
        }
      }
    }
    return getCheckLivenessMethod;
  }

  private static volatile io.grpc.MethodDescriptor<io.grps.protos.GrpsProtos.GrpsMessage,
      io.grps.protos.GrpsProtos.GrpsMessage> getCheckReadinessMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "CheckReadiness",
      requestType = io.grps.protos.GrpsProtos.GrpsMessage.class,
      responseType = io.grps.protos.GrpsProtos.GrpsMessage.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<io.grps.protos.GrpsProtos.GrpsMessage,
      io.grps.protos.GrpsProtos.GrpsMessage> getCheckReadinessMethod() {
    io.grpc.MethodDescriptor<io.grps.protos.GrpsProtos.GrpsMessage, io.grps.protos.GrpsProtos.GrpsMessage> getCheckReadinessMethod;
    if ((getCheckReadinessMethod = GrpsServiceGrpc.getCheckReadinessMethod) == null) {
      synchronized (GrpsServiceGrpc.class) {
        if ((getCheckReadinessMethod = GrpsServiceGrpc.getCheckReadinessMethod) == null) {
          GrpsServiceGrpc.getCheckReadinessMethod = getCheckReadinessMethod =
              io.grpc.MethodDescriptor.<io.grps.protos.GrpsProtos.GrpsMessage, io.grps.protos.GrpsProtos.GrpsMessage>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "CheckReadiness"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  io.grps.protos.GrpsProtos.GrpsMessage.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  io.grps.protos.GrpsProtos.GrpsMessage.getDefaultInstance()))
              .setSchemaDescriptor(new GrpsServiceMethodDescriptorSupplier("CheckReadiness"))
              .build();
        }
      }
    }
    return getCheckReadinessMethod;
  }

  private static volatile io.grpc.MethodDescriptor<io.grps.protos.GrpsProtos.GrpsMessage,
      io.grps.protos.GrpsProtos.GrpsMessage> getServerMetadataMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "ServerMetadata",
      requestType = io.grps.protos.GrpsProtos.GrpsMessage.class,
      responseType = io.grps.protos.GrpsProtos.GrpsMessage.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<io.grps.protos.GrpsProtos.GrpsMessage,
      io.grps.protos.GrpsProtos.GrpsMessage> getServerMetadataMethod() {
    io.grpc.MethodDescriptor<io.grps.protos.GrpsProtos.GrpsMessage, io.grps.protos.GrpsProtos.GrpsMessage> getServerMetadataMethod;
    if ((getServerMetadataMethod = GrpsServiceGrpc.getServerMetadataMethod) == null) {
      synchronized (GrpsServiceGrpc.class) {
        if ((getServerMetadataMethod = GrpsServiceGrpc.getServerMetadataMethod) == null) {
          GrpsServiceGrpc.getServerMetadataMethod = getServerMetadataMethod =
              io.grpc.MethodDescriptor.<io.grps.protos.GrpsProtos.GrpsMessage, io.grps.protos.GrpsProtos.GrpsMessage>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "ServerMetadata"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  io.grps.protos.GrpsProtos.GrpsMessage.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  io.grps.protos.GrpsProtos.GrpsMessage.getDefaultInstance()))
              .setSchemaDescriptor(new GrpsServiceMethodDescriptorSupplier("ServerMetadata"))
              .build();
        }
      }
    }
    return getServerMetadataMethod;
  }

  private static volatile io.grpc.MethodDescriptor<io.grps.protos.GrpsProtos.GrpsMessage,
      io.grps.protos.GrpsProtos.GrpsMessage> getModelMetadataMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "ModelMetadata",
      requestType = io.grps.protos.GrpsProtos.GrpsMessage.class,
      responseType = io.grps.protos.GrpsProtos.GrpsMessage.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<io.grps.protos.GrpsProtos.GrpsMessage,
      io.grps.protos.GrpsProtos.GrpsMessage> getModelMetadataMethod() {
    io.grpc.MethodDescriptor<io.grps.protos.GrpsProtos.GrpsMessage, io.grps.protos.GrpsProtos.GrpsMessage> getModelMetadataMethod;
    if ((getModelMetadataMethod = GrpsServiceGrpc.getModelMetadataMethod) == null) {
      synchronized (GrpsServiceGrpc.class) {
        if ((getModelMetadataMethod = GrpsServiceGrpc.getModelMetadataMethod) == null) {
          GrpsServiceGrpc.getModelMetadataMethod = getModelMetadataMethod =
              io.grpc.MethodDescriptor.<io.grps.protos.GrpsProtos.GrpsMessage, io.grps.protos.GrpsProtos.GrpsMessage>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "ModelMetadata"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  io.grps.protos.GrpsProtos.GrpsMessage.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  io.grps.protos.GrpsProtos.GrpsMessage.getDefaultInstance()))
              .setSchemaDescriptor(new GrpsServiceMethodDescriptorSupplier("ModelMetadata"))
              .build();
        }
      }
    }
    return getModelMetadataMethod;
  }

  /**
   * Creates a new async stub that supports all call types for the service
   */
  public static GrpsServiceStub newStub(io.grpc.Channel channel) {
    io.grpc.stub.AbstractStub.StubFactory<GrpsServiceStub> factory =
      new io.grpc.stub.AbstractStub.StubFactory<GrpsServiceStub>() {
        @java.lang.Override
        public GrpsServiceStub newStub(io.grpc.Channel channel, io.grpc.CallOptions callOptions) {
          return new GrpsServiceStub(channel, callOptions);
        }
      };
    return GrpsServiceStub.newStub(factory, channel);
  }

  /**
   * Creates a new blocking-style stub that supports unary and streaming output calls on the service
   */
  public static GrpsServiceBlockingStub newBlockingStub(
      io.grpc.Channel channel) {
    io.grpc.stub.AbstractStub.StubFactory<GrpsServiceBlockingStub> factory =
      new io.grpc.stub.AbstractStub.StubFactory<GrpsServiceBlockingStub>() {
        @java.lang.Override
        public GrpsServiceBlockingStub newStub(io.grpc.Channel channel, io.grpc.CallOptions callOptions) {
          return new GrpsServiceBlockingStub(channel, callOptions);
        }
      };
    return GrpsServiceBlockingStub.newStub(factory, channel);
  }

  /**
   * Creates a new ListenableFuture-style stub that supports unary calls on the service
   */
  public static GrpsServiceFutureStub newFutureStub(
      io.grpc.Channel channel) {
    io.grpc.stub.AbstractStub.StubFactory<GrpsServiceFutureStub> factory =
      new io.grpc.stub.AbstractStub.StubFactory<GrpsServiceFutureStub>() {
        @java.lang.Override
        public GrpsServiceFutureStub newStub(io.grpc.Channel channel, io.grpc.CallOptions callOptions) {
          return new GrpsServiceFutureStub(channel, callOptions);
        }
      };
    return GrpsServiceFutureStub.newStub(factory, channel);
  }

  /**
   */
  public static abstract class GrpsServiceImplBase implements io.grpc.BindableService {

    /**
     */
    public void predict(io.grps.protos.GrpsProtos.GrpsMessage request,
        io.grpc.stub.StreamObserver<io.grps.protos.GrpsProtos.GrpsMessage> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getPredictMethod(), responseObserver);
    }

    /**
     */
    public void predictStreaming(io.grps.protos.GrpsProtos.GrpsMessage request,
        io.grpc.stub.StreamObserver<io.grps.protos.GrpsProtos.GrpsMessage> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getPredictStreamingMethod(), responseObserver);
    }

    /**
     */
    public void online(io.grps.protos.GrpsProtos.GrpsMessage request,
        io.grpc.stub.StreamObserver<io.grps.protos.GrpsProtos.GrpsMessage> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getOnlineMethod(), responseObserver);
    }

    /**
     */
    public void offline(io.grps.protos.GrpsProtos.GrpsMessage request,
        io.grpc.stub.StreamObserver<io.grps.protos.GrpsProtos.GrpsMessage> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getOfflineMethod(), responseObserver);
    }

    /**
     */
    public void checkLiveness(io.grps.protos.GrpsProtos.GrpsMessage request,
        io.grpc.stub.StreamObserver<io.grps.protos.GrpsProtos.GrpsMessage> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getCheckLivenessMethod(), responseObserver);
    }

    /**
     */
    public void checkReadiness(io.grps.protos.GrpsProtos.GrpsMessage request,
        io.grpc.stub.StreamObserver<io.grps.protos.GrpsProtos.GrpsMessage> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getCheckReadinessMethod(), responseObserver);
    }

    /**
     */
    public void serverMetadata(io.grps.protos.GrpsProtos.GrpsMessage request,
        io.grpc.stub.StreamObserver<io.grps.protos.GrpsProtos.GrpsMessage> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getServerMetadataMethod(), responseObserver);
    }

    /**
     */
    public void modelMetadata(io.grps.protos.GrpsProtos.GrpsMessage request,
        io.grpc.stub.StreamObserver<io.grps.protos.GrpsProtos.GrpsMessage> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getModelMetadataMethod(), responseObserver);
    }

    @java.lang.Override public final io.grpc.ServerServiceDefinition bindService() {
      return io.grpc.ServerServiceDefinition.builder(getServiceDescriptor())
          .addMethod(
            getPredictMethod(),
            io.grpc.stub.ServerCalls.asyncUnaryCall(
              new MethodHandlers<
                io.grps.protos.GrpsProtos.GrpsMessage,
                io.grps.protos.GrpsProtos.GrpsMessage>(
                  this, METHODID_PREDICT)))
          .addMethod(
            getPredictStreamingMethod(),
            io.grpc.stub.ServerCalls.asyncServerStreamingCall(
              new MethodHandlers<
                io.grps.protos.GrpsProtos.GrpsMessage,
                io.grps.protos.GrpsProtos.GrpsMessage>(
                  this, METHODID_PREDICT_STREAMING)))
          .addMethod(
            getOnlineMethod(),
            io.grpc.stub.ServerCalls.asyncUnaryCall(
              new MethodHandlers<
                io.grps.protos.GrpsProtos.GrpsMessage,
                io.grps.protos.GrpsProtos.GrpsMessage>(
                  this, METHODID_ONLINE)))
          .addMethod(
            getOfflineMethod(),
            io.grpc.stub.ServerCalls.asyncUnaryCall(
              new MethodHandlers<
                io.grps.protos.GrpsProtos.GrpsMessage,
                io.grps.protos.GrpsProtos.GrpsMessage>(
                  this, METHODID_OFFLINE)))
          .addMethod(
            getCheckLivenessMethod(),
            io.grpc.stub.ServerCalls.asyncUnaryCall(
              new MethodHandlers<
                io.grps.protos.GrpsProtos.GrpsMessage,
                io.grps.protos.GrpsProtos.GrpsMessage>(
                  this, METHODID_CHECK_LIVENESS)))
          .addMethod(
            getCheckReadinessMethod(),
            io.grpc.stub.ServerCalls.asyncUnaryCall(
              new MethodHandlers<
                io.grps.protos.GrpsProtos.GrpsMessage,
                io.grps.protos.GrpsProtos.GrpsMessage>(
                  this, METHODID_CHECK_READINESS)))
          .addMethod(
            getServerMetadataMethod(),
            io.grpc.stub.ServerCalls.asyncUnaryCall(
              new MethodHandlers<
                io.grps.protos.GrpsProtos.GrpsMessage,
                io.grps.protos.GrpsProtos.GrpsMessage>(
                  this, METHODID_SERVER_METADATA)))
          .addMethod(
            getModelMetadataMethod(),
            io.grpc.stub.ServerCalls.asyncUnaryCall(
              new MethodHandlers<
                io.grps.protos.GrpsProtos.GrpsMessage,
                io.grps.protos.GrpsProtos.GrpsMessage>(
                  this, METHODID_MODEL_METADATA)))
          .build();
    }
  }

  /**
   */
  public static final class GrpsServiceStub extends io.grpc.stub.AbstractAsyncStub<GrpsServiceStub> {
    private GrpsServiceStub(
        io.grpc.Channel channel, io.grpc.CallOptions callOptions) {
      super(channel, callOptions);
    }

    @java.lang.Override
    protected GrpsServiceStub build(
        io.grpc.Channel channel, io.grpc.CallOptions callOptions) {
      return new GrpsServiceStub(channel, callOptions);
    }

    /**
     */
    public void predict(io.grps.protos.GrpsProtos.GrpsMessage request,
        io.grpc.stub.StreamObserver<io.grps.protos.GrpsProtos.GrpsMessage> responseObserver) {
      io.grpc.stub.ClientCalls.asyncUnaryCall(
          getChannel().newCall(getPredictMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     */
    public void predictStreaming(io.grps.protos.GrpsProtos.GrpsMessage request,
        io.grpc.stub.StreamObserver<io.grps.protos.GrpsProtos.GrpsMessage> responseObserver) {
      io.grpc.stub.ClientCalls.asyncServerStreamingCall(
          getChannel().newCall(getPredictStreamingMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     */
    public void online(io.grps.protos.GrpsProtos.GrpsMessage request,
        io.grpc.stub.StreamObserver<io.grps.protos.GrpsProtos.GrpsMessage> responseObserver) {
      io.grpc.stub.ClientCalls.asyncUnaryCall(
          getChannel().newCall(getOnlineMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     */
    public void offline(io.grps.protos.GrpsProtos.GrpsMessage request,
        io.grpc.stub.StreamObserver<io.grps.protos.GrpsProtos.GrpsMessage> responseObserver) {
      io.grpc.stub.ClientCalls.asyncUnaryCall(
          getChannel().newCall(getOfflineMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     */
    public void checkLiveness(io.grps.protos.GrpsProtos.GrpsMessage request,
        io.grpc.stub.StreamObserver<io.grps.protos.GrpsProtos.GrpsMessage> responseObserver) {
      io.grpc.stub.ClientCalls.asyncUnaryCall(
          getChannel().newCall(getCheckLivenessMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     */
    public void checkReadiness(io.grps.protos.GrpsProtos.GrpsMessage request,
        io.grpc.stub.StreamObserver<io.grps.protos.GrpsProtos.GrpsMessage> responseObserver) {
      io.grpc.stub.ClientCalls.asyncUnaryCall(
          getChannel().newCall(getCheckReadinessMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     */
    public void serverMetadata(io.grps.protos.GrpsProtos.GrpsMessage request,
        io.grpc.stub.StreamObserver<io.grps.protos.GrpsProtos.GrpsMessage> responseObserver) {
      io.grpc.stub.ClientCalls.asyncUnaryCall(
          getChannel().newCall(getServerMetadataMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     */
    public void modelMetadata(io.grps.protos.GrpsProtos.GrpsMessage request,
        io.grpc.stub.StreamObserver<io.grps.protos.GrpsProtos.GrpsMessage> responseObserver) {
      io.grpc.stub.ClientCalls.asyncUnaryCall(
          getChannel().newCall(getModelMetadataMethod(), getCallOptions()), request, responseObserver);
    }
  }

  /**
   */
  public static final class GrpsServiceBlockingStub extends io.grpc.stub.AbstractBlockingStub<GrpsServiceBlockingStub> {
    private GrpsServiceBlockingStub(
        io.grpc.Channel channel, io.grpc.CallOptions callOptions) {
      super(channel, callOptions);
    }

    @java.lang.Override
    protected GrpsServiceBlockingStub build(
        io.grpc.Channel channel, io.grpc.CallOptions callOptions) {
      return new GrpsServiceBlockingStub(channel, callOptions);
    }

    /**
     */
    public io.grps.protos.GrpsProtos.GrpsMessage predict(io.grps.protos.GrpsProtos.GrpsMessage request) {
      return io.grpc.stub.ClientCalls.blockingUnaryCall(
          getChannel(), getPredictMethod(), getCallOptions(), request);
    }

    /**
     */
    public java.util.Iterator<io.grps.protos.GrpsProtos.GrpsMessage> predictStreaming(
        io.grps.protos.GrpsProtos.GrpsMessage request) {
      return io.grpc.stub.ClientCalls.blockingServerStreamingCall(
          getChannel(), getPredictStreamingMethod(), getCallOptions(), request);
    }

    /**
     */
    public io.grps.protos.GrpsProtos.GrpsMessage online(io.grps.protos.GrpsProtos.GrpsMessage request) {
      return io.grpc.stub.ClientCalls.blockingUnaryCall(
          getChannel(), getOnlineMethod(), getCallOptions(), request);
    }

    /**
     */
    public io.grps.protos.GrpsProtos.GrpsMessage offline(io.grps.protos.GrpsProtos.GrpsMessage request) {
      return io.grpc.stub.ClientCalls.blockingUnaryCall(
          getChannel(), getOfflineMethod(), getCallOptions(), request);
    }

    /**
     */
    public io.grps.protos.GrpsProtos.GrpsMessage checkLiveness(io.grps.protos.GrpsProtos.GrpsMessage request) {
      return io.grpc.stub.ClientCalls.blockingUnaryCall(
          getChannel(), getCheckLivenessMethod(), getCallOptions(), request);
    }

    /**
     */
    public io.grps.protos.GrpsProtos.GrpsMessage checkReadiness(io.grps.protos.GrpsProtos.GrpsMessage request) {
      return io.grpc.stub.ClientCalls.blockingUnaryCall(
          getChannel(), getCheckReadinessMethod(), getCallOptions(), request);
    }

    /**
     */
    public io.grps.protos.GrpsProtos.GrpsMessage serverMetadata(io.grps.protos.GrpsProtos.GrpsMessage request) {
      return io.grpc.stub.ClientCalls.blockingUnaryCall(
          getChannel(), getServerMetadataMethod(), getCallOptions(), request);
    }

    /**
     */
    public io.grps.protos.GrpsProtos.GrpsMessage modelMetadata(io.grps.protos.GrpsProtos.GrpsMessage request) {
      return io.grpc.stub.ClientCalls.blockingUnaryCall(
          getChannel(), getModelMetadataMethod(), getCallOptions(), request);
    }
  }

  /**
   */
  public static final class GrpsServiceFutureStub extends io.grpc.stub.AbstractFutureStub<GrpsServiceFutureStub> {
    private GrpsServiceFutureStub(
        io.grpc.Channel channel, io.grpc.CallOptions callOptions) {
      super(channel, callOptions);
    }

    @java.lang.Override
    protected GrpsServiceFutureStub build(
        io.grpc.Channel channel, io.grpc.CallOptions callOptions) {
      return new GrpsServiceFutureStub(channel, callOptions);
    }

    /**
     */
    public com.google.common.util.concurrent.ListenableFuture<io.grps.protos.GrpsProtos.GrpsMessage> predict(
        io.grps.protos.GrpsProtos.GrpsMessage request) {
      return io.grpc.stub.ClientCalls.futureUnaryCall(
          getChannel().newCall(getPredictMethod(), getCallOptions()), request);
    }

    /**
     */
    public com.google.common.util.concurrent.ListenableFuture<io.grps.protos.GrpsProtos.GrpsMessage> online(
        io.grps.protos.GrpsProtos.GrpsMessage request) {
      return io.grpc.stub.ClientCalls.futureUnaryCall(
          getChannel().newCall(getOnlineMethod(), getCallOptions()), request);
    }

    /**
     */
    public com.google.common.util.concurrent.ListenableFuture<io.grps.protos.GrpsProtos.GrpsMessage> offline(
        io.grps.protos.GrpsProtos.GrpsMessage request) {
      return io.grpc.stub.ClientCalls.futureUnaryCall(
          getChannel().newCall(getOfflineMethod(), getCallOptions()), request);
    }

    /**
     */
    public com.google.common.util.concurrent.ListenableFuture<io.grps.protos.GrpsProtos.GrpsMessage> checkLiveness(
        io.grps.protos.GrpsProtos.GrpsMessage request) {
      return io.grpc.stub.ClientCalls.futureUnaryCall(
          getChannel().newCall(getCheckLivenessMethod(), getCallOptions()), request);
    }

    /**
     */
    public com.google.common.util.concurrent.ListenableFuture<io.grps.protos.GrpsProtos.GrpsMessage> checkReadiness(
        io.grps.protos.GrpsProtos.GrpsMessage request) {
      return io.grpc.stub.ClientCalls.futureUnaryCall(
          getChannel().newCall(getCheckReadinessMethod(), getCallOptions()), request);
    }

    /**
     */
    public com.google.common.util.concurrent.ListenableFuture<io.grps.protos.GrpsProtos.GrpsMessage> serverMetadata(
        io.grps.protos.GrpsProtos.GrpsMessage request) {
      return io.grpc.stub.ClientCalls.futureUnaryCall(
          getChannel().newCall(getServerMetadataMethod(), getCallOptions()), request);
    }

    /**
     */
    public com.google.common.util.concurrent.ListenableFuture<io.grps.protos.GrpsProtos.GrpsMessage> modelMetadata(
        io.grps.protos.GrpsProtos.GrpsMessage request) {
      return io.grpc.stub.ClientCalls.futureUnaryCall(
          getChannel().newCall(getModelMetadataMethod(), getCallOptions()), request);
    }
  }

  private static final int METHODID_PREDICT = 0;
  private static final int METHODID_PREDICT_STREAMING = 1;
  private static final int METHODID_ONLINE = 2;
  private static final int METHODID_OFFLINE = 3;
  private static final int METHODID_CHECK_LIVENESS = 4;
  private static final int METHODID_CHECK_READINESS = 5;
  private static final int METHODID_SERVER_METADATA = 6;
  private static final int METHODID_MODEL_METADATA = 7;

  private static final class MethodHandlers<Req, Resp> implements
      io.grpc.stub.ServerCalls.UnaryMethod<Req, Resp>,
      io.grpc.stub.ServerCalls.ServerStreamingMethod<Req, Resp>,
      io.grpc.stub.ServerCalls.ClientStreamingMethod<Req, Resp>,
      io.grpc.stub.ServerCalls.BidiStreamingMethod<Req, Resp> {
    private final GrpsServiceImplBase serviceImpl;
    private final int methodId;

    MethodHandlers(GrpsServiceImplBase serviceImpl, int methodId) {
      this.serviceImpl = serviceImpl;
      this.methodId = methodId;
    }

    @java.lang.Override
    @java.lang.SuppressWarnings("unchecked")
    public void invoke(Req request, io.grpc.stub.StreamObserver<Resp> responseObserver) {
      switch (methodId) {
        case METHODID_PREDICT:
          serviceImpl.predict((io.grps.protos.GrpsProtos.GrpsMessage) request,
              (io.grpc.stub.StreamObserver<io.grps.protos.GrpsProtos.GrpsMessage>) responseObserver);
          break;
        case METHODID_PREDICT_STREAMING:
          serviceImpl.predictStreaming((io.grps.protos.GrpsProtos.GrpsMessage) request,
              (io.grpc.stub.StreamObserver<io.grps.protos.GrpsProtos.GrpsMessage>) responseObserver);
          break;
        case METHODID_ONLINE:
          serviceImpl.online((io.grps.protos.GrpsProtos.GrpsMessage) request,
              (io.grpc.stub.StreamObserver<io.grps.protos.GrpsProtos.GrpsMessage>) responseObserver);
          break;
        case METHODID_OFFLINE:
          serviceImpl.offline((io.grps.protos.GrpsProtos.GrpsMessage) request,
              (io.grpc.stub.StreamObserver<io.grps.protos.GrpsProtos.GrpsMessage>) responseObserver);
          break;
        case METHODID_CHECK_LIVENESS:
          serviceImpl.checkLiveness((io.grps.protos.GrpsProtos.GrpsMessage) request,
              (io.grpc.stub.StreamObserver<io.grps.protos.GrpsProtos.GrpsMessage>) responseObserver);
          break;
        case METHODID_CHECK_READINESS:
          serviceImpl.checkReadiness((io.grps.protos.GrpsProtos.GrpsMessage) request,
              (io.grpc.stub.StreamObserver<io.grps.protos.GrpsProtos.GrpsMessage>) responseObserver);
          break;
        case METHODID_SERVER_METADATA:
          serviceImpl.serverMetadata((io.grps.protos.GrpsProtos.GrpsMessage) request,
              (io.grpc.stub.StreamObserver<io.grps.protos.GrpsProtos.GrpsMessage>) responseObserver);
          break;
        case METHODID_MODEL_METADATA:
          serviceImpl.modelMetadata((io.grps.protos.GrpsProtos.GrpsMessage) request,
              (io.grpc.stub.StreamObserver<io.grps.protos.GrpsProtos.GrpsMessage>) responseObserver);
          break;
        default:
          throw new AssertionError();
      }
    }

    @java.lang.Override
    @java.lang.SuppressWarnings("unchecked")
    public io.grpc.stub.StreamObserver<Req> invoke(
        io.grpc.stub.StreamObserver<Resp> responseObserver) {
      switch (methodId) {
        default:
          throw new AssertionError();
      }
    }
  }

  private static abstract class GrpsServiceBaseDescriptorSupplier
      implements io.grpc.protobuf.ProtoFileDescriptorSupplier, io.grpc.protobuf.ProtoServiceDescriptorSupplier {
    GrpsServiceBaseDescriptorSupplier() {}

    @java.lang.Override
    public com.google.protobuf.Descriptors.FileDescriptor getFileDescriptor() {
      return io.grps.protos.GrpsProtos.getDescriptor();
    }

    @java.lang.Override
    public com.google.protobuf.Descriptors.ServiceDescriptor getServiceDescriptor() {
      return getFileDescriptor().findServiceByName("GrpsService");
    }
  }

  private static final class GrpsServiceFileDescriptorSupplier
      extends GrpsServiceBaseDescriptorSupplier {
    GrpsServiceFileDescriptorSupplier() {}
  }

  private static final class GrpsServiceMethodDescriptorSupplier
      extends GrpsServiceBaseDescriptorSupplier
      implements io.grpc.protobuf.ProtoMethodDescriptorSupplier {
    private final String methodName;

    GrpsServiceMethodDescriptorSupplier(String methodName) {
      this.methodName = methodName;
    }

    @java.lang.Override
    public com.google.protobuf.Descriptors.MethodDescriptor getMethodDescriptor() {
      return getServiceDescriptor().findMethodByName(methodName);
    }
  }

  private static volatile io.grpc.ServiceDescriptor serviceDescriptor;

  public static io.grpc.ServiceDescriptor getServiceDescriptor() {
    io.grpc.ServiceDescriptor result = serviceDescriptor;
    if (result == null) {
      synchronized (GrpsServiceGrpc.class) {
        result = serviceDescriptor;
        if (result == null) {
          serviceDescriptor = result = io.grpc.ServiceDescriptor.newBuilder(SERVICE_NAME)
              .setSchemaDescriptor(new GrpsServiceFileDescriptorSupplier())
              .addMethod(getPredictMethod())
              .addMethod(getPredictStreamingMethod())
              .addMethod(getOnlineMethod())
              .addMethod(getOfflineMethod())
              .addMethod(getCheckLivenessMethod())
              .addMethod(getCheckReadinessMethod())
              .addMethod(getServerMetadataMethod())
              .addMethod(getModelMetadataMethod())
              .build();
        }
      }
    }
    return result;
  }
}
