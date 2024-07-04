# FAQ

## 服务部署路径在哪？

部署路径会以服务名命名（```grpst start``` --name参数可以指定服务名），即不同服务名的服务的部署路径不同，可以通过`grpst ps`
命令查看所有已启动的部署路径。

```
# 查看服务信息：
grpst ps
# 输出如下：
PORT(HTTP,RPC)      NAME                PID                 DEPLOY_PATH
7080,7081           my_grps             12345               /root/.grps/my_grps
```

## C++自定义工程grps服务崩溃如何分析？

### 查看backtrace

服务崩溃后会自动打印backtrace信息并打印代码行号以及附近代码信息，可以通过`grpst logs`命令查看，如下：

```
# 查看服务日志
grpst logs my_grps

# 如下越界导致的崩溃backtrace信息：
==> /root/.grps/my_grps/nohup.out <==
#1    Source "/root/my_grps4/cpp_server/src/dag/node.cc", line 38, in Process [0x55b76973527b]
#0    Source "/root/my_grps4/src/customized_converter.cc", line 61, in PostProcess [0x7f323961444a]
         58:   auto& your_value = ctx.GetUserData<YourCtxVal>(); // Use reference to avoid copy.
         59:   CLOG4(INFO, "your context user data value: " << your_value.val);
         60:   std::vector<int> arr;
      >  61:   int ret = arr[100];
         62:   CLOG4(INFO, "" << ret);
         63:
         64:   // Postprocess tensors and convert to response.
Segmentation fault (Address not mapped to object [0x190])


# 也可以进入部署路径查看终端日志
# 进入部署路径
cd /root/.grps/my_grps

# 查看终端日志
tail -f nohup.out
```

### gdb分析

如果系统打开了core dump文件生成开关，可以在服务崩溃后进入部署路径，使用gdb调试core文件，如下：

```
# 进入部署路径
cd /root/.grps/my_grps

# gdb调试
gdb ./bin/grps_server core
```

## Python自定义工程grps服务如何加入断点调试？

一般来说我们并不需要进行grps服务完整工程的调试，我们可以通过单测构建请求输入和检查输出即可，我们构建的输入即是服务部署后请求的输入，输出即是返回的输出。对于单测运行的请求处理代码和服务部署后运行的请求处理代码是一套framework代码，因此我们无需担心两种方式引入的差异。
某些情况下（例如streaming模式）如果我们确实需要进行完整的服务调试，则可以通过如下方式进行：

1. 通过pdb方式加入一些断点。
2. 通过`grpst archive`打包，然后通过`grpst start`启动一次服务，使用`grpst ps`命令查看服务信息，如下：

```
# 构建：
grpst archive .

# 启动：
grpst start server.mar

# 查看服务信息：
grpst ps
# 输出如下：
PORT(HTTP,RPC)      NAME                PID                 DEPLOY_PATH
7080,7081           my_grps             12345               /root/.grps/my_grps
```

3. 使用`grpst stop`命令关闭后转为手动启动，如下：

```
# 关闭服务：
grpst stop my_grps

# 进入部署路径
cd /root/.grps/my_grps

# 手动启动服务，进行断点调试
python3 grps_framework/main.py
```

这里需要注意的一点是`grpst start`命令每次启动会覆盖部署路径的内容。
