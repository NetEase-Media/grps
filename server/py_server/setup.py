# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2023/9/3
# Brief  Build grps framework pip package.

from setuptools import setup, find_packages

setup(
    name='grps_framework',
    version='1.1.0',
    description='grps framework',
    author='netease media',
    packages=find_packages(include=['grps_framework', 'grps_framework.*']),
    include_package_data=True,
    # install_requires=['tensorflow>=2.0.0', 'torch>=1.8.0', 'grpcio>=1.41.0', 'grpcio-tools>=1.41.0', 'PyYAML>=6.0.1']
)
