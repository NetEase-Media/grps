# Copyright 2022 netease. All rights reserved.
# Author zhaochaochao@corp.netease.com
# Date   2023/4/10
# Brief  Generic Realtime Prediction Service Tools.
import argparse
import os
import sys
from sys import exit

from archive.archive import GrpsProjectArchiver
from const import GRPS_VERSION
from create.create import GrpsProjectCreator
from deploy.deploy import GrpsProjectDeployer
from logs.logs import GrpsLogs
from ps.ps import GrpsPrintStatus
from quick_deploy.quick_serve import GrpsQuickServe
from stop.stop import GrpsProjectStopper


def build_arg_parser():
    parser = argparse.ArgumentParser(prog='grpst', description='Generic Realtime Prediction Service Tools.')
    subparsers = parser.add_subparsers(help='sub-command help')

    # Add create sub-command.
    creator = GrpsProjectCreator()
    creator.build_parser(subparsers)

    # Add archive sub-command.
    archiver = GrpsProjectArchiver()
    archiver.build_parser(subparsers)

    # Add start sub-command.
    deployer = GrpsProjectDeployer()
    deployer.build_parser(subparsers)

    # Add quick serve sub-command.
    quick_serve = GrpsQuickServe()
    quick_serve.build_parser(subparsers)

    # Add stop serve sub-command.
    quick_stop = GrpsProjectStopper()
    quick_stop.build_parser(subparsers)

    # Add ps sub-command.
    ps = GrpsPrintStatus()
    ps.build_parser(subparsers)

    # Add logs sub-command.
    logs = GrpsLogs()
    logs.build_parser(subparsers)

    # Add version sub-command.
    parser.add_argument('-v', '--version', action='version', version='%(prog)s ' + GRPS_VERSION)
    return parser


if __name__ == '__main__':
    # Build arg parser.
    parser = build_arg_parser()
    if len(sys.argv) < 2:
        parser.print_help()
        exit(0)
    else:
        # Parse args.
        args = parser.parse_args()
        exit(args.func(args))
