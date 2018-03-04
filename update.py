#!/usr/bin/env python
import os
import sys
import subprocess

subprocess.Popen('svn up ../../include/StorageEngine',shell=True).wait()
subprocess.Popen('svn up .' ,shell=True).wait();
