# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2023/9/15
# Brief  grps_apis lib __init__
import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))

from . import grps_pb2, grps_pb2_grpc

__all__ = ['grps_pb2', 'grps_pb2_grpc']
