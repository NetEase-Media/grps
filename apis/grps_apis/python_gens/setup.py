# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2023/9/3
# Brief  Build grps apis pip package.

from setuptools import setup, find_packages

setup(
    name='grps_apis',
    version='1.1.0',
    description='grps apis',
    author='netease media',
    packages=find_packages(include=['grps_apis', 'grps_apis.*']),
    include_package_data=True,
    install_requires=['grpcio>=1.46.0', 'grpcio-tools>=1.46.0']
)
