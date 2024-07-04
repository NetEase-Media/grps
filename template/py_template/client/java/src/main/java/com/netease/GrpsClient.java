// Java client demo. Complete interface description can be learned from docs/2_Interface.md.

package com.netease;

import com.google.common.base.Charsets;
import com.google.common.collect.Lists;
import com.google.protobuf.ByteString;
import io.grpc.Channel;
import io.grpc.Grpc;
import io.grpc.InsecureChannelCredentials;
import io.grpc.ManagedChannel;
import io.grps.protos.GrpsProtos;
import io.grps.protos.GrpsServiceGrpc;
import io.grpc.stub.StreamObserver;

import java.io.UnsupportedEncodingException;

public class GrpsClient {
    private final GrpsServiceGrpc.GrpsServiceBlockingStub blockingStub;
    private final GrpsServiceGrpc.GrpsServiceStub asyncStub;

    public GrpsClient(Channel channel) {
        blockingStub = GrpsServiceGrpc.newBlockingStub(channel);
        asyncStub = GrpsServiceGrpc.newStub(channel);
    }

    public void checkLiveNess() {
        final GrpsProtos.GrpsMessage grpsMessage = blockingStub.checkLiveness(GrpsProtos.GrpsMessage.newBuilder().build());
        System.out.println(grpsMessage.toString());
    }

    public void onlineServer() {
        final GrpsProtos.GrpsMessage grpsMessage = blockingStub.online(GrpsProtos.GrpsMessage.newBuilder().build());
        System.out.println(grpsMessage.toString());
    }

    public void checkReadiness() {
        final GrpsProtos.GrpsMessage grpsMessage = blockingStub.checkReadiness(GrpsProtos.GrpsMessage.newBuilder().build());
        System.out.println(grpsMessage.toString());
    }

    public void predictWithStr() {
        final GrpsProtos.GrpsMessage grpsMessage = blockingStub.predict(GrpsProtos.GrpsMessage.newBuilder().setStrData("hello grps.").build());
        System.out.println(grpsMessage.toString());
    }

    public void predictStreaming() {
        StreamObserver<GrpsProtos.GrpsMessage> responseObserver = new StreamObserver<GrpsProtos.GrpsMessage>() {
            @Override
            public void onNext(GrpsProtos.GrpsMessage value) {
                System.out.println("onNext: " + value.toString());
            }

            @Override
            public void onError(Throwable t) {
                System.out.println("onError: " + t.getMessage());
            }

            @Override
            public void onCompleted() {
                System.out.println("onCompleted");
            }
        };

        asyncStub.predictStreaming(GrpsProtos.GrpsMessage.newBuilder().setStrData("hello grps.").build(), responseObserver);
        try {
            Thread.sleep(1000);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    public void predictWithGTensors() {
        GrpsProtos.GenericTensor gtensor = GrpsProtos.GenericTensor.newBuilder()
                .setName("inp")
                .addAllShape(Lists.newArrayList(2, 3))
                .setDtype(GrpsProtos.DataType.DT_FLOAT32)
                .addAllFlatFloat32(Lists.newArrayList(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f))
                .build();
        GrpsProtos.GenericTensorData genericTensorData = GrpsProtos.GenericTensorData.newBuilder()
                .addTensors(gtensor)
                .build();
        final GrpsProtos.GrpsMessage grpsMessage =
                blockingStub.predict(GrpsProtos.GrpsMessage.newBuilder().setGtensors(genericTensorData).build());
        System.out.println(grpsMessage.toString());
    }

    public void predictWithBinData() throws UnsupportedEncodingException {
        final GrpsProtos.GrpsMessage grpsMessage =
                blockingStub.predict(GrpsProtos.GrpsMessage.newBuilder()
                        .setBinData(ByteString.copyFrom("hello grps.", Charsets.UTF_8)).build());
        System.out.println(grpsMessage.toString());
    }

    public void serverMetaData() {
        final GrpsProtos.GrpsMessage grpsMessage = blockingStub.serverMetadata(GrpsProtos.GrpsMessage.newBuilder().build());
        System.out.println(grpsMessage.toString());
    }

    public void modelMetaData() {
        final GrpsProtos.GrpsMessage grpsMessage = blockingStub.modelMetadata(GrpsProtos.GrpsMessage.newBuilder()
                .setStrData("your_model").build());
        System.out.println(grpsMessage.toString());
    }

    public void offline() {
        final GrpsProtos.GrpsMessage grpsMessage = blockingStub.offline(GrpsProtos.GrpsMessage.newBuilder().build());
        System.out.println(grpsMessage.toString());
    }

    public static void main(String[] args) throws Exception {
        String target = "0.0.0.0:7081";
        if (args != null && args.length > 0) {
            target = args[0];
        }
        System.out.println("grpc target: " + target);
        ManagedChannel channel = Grpc.newChannelBuilder(target, InsecureChannelCredentials.create())
                .build();
        try {
            GrpsClient client = new GrpsClient(channel);
            client.checkLiveNess();
            client.onlineServer();
            client.checkReadiness();
            client.predictWithStr();
            client.predictWithGTensors();
            client.predictWithBinData();
            client.predictStreaming();
            client.serverMetaData();
            client.modelMetaData();
            client.offline();
        } finally {
            channel.shutdownNow().awaitTermination(5, java.util.concurrent.TimeUnit.SECONDS);
        }
    }

}
