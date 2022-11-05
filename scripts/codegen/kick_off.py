# coding=utf-8
import os, sys, shutil, argparse

from datetime import datetime
from string import Template
from ticos_thingmodel_gen import generate as hal_generator

DST_DIR = 'ticos_thingmodel'
INSTLER = 'install.' 'sh' # TODO the file ext name should be detected by os
MQTTHAL = 'mqtt_wrapper.c'

def copy_file(src, dst):
    s = ''
    with open(src, 'r') as f:
        s = f.read()
    if not s:
        raise Exception('文件读取失败： %s' % src)
    with open(dst, 'w') as f:
        f.write(s)

def gen_codes(ext, tmpl, thingmodel, to):
    if thingmodel:
        copy_file(tmpl + '/README.md', to + '/README.md')
        copy_file(tmpl + '/' + MQTTHAL, to + '/' + MQTTHAL)
        hal_generator(thingmodel, to)

def gen_for_esp32(tmpl, thingmodel, to):
    copy_file(tmpl + '/tools/' + INSTLER, to + '/' + INSTLER)
    gen_codes('c', tmpl, thingmodel, to)

def gen_for_arduino(tmpl, thingmodel, to):
    gen_codes('ino', tmpl, thingmodel, to)

def generate(name, platform, thingmodel='', to='.'):
    if not platform:
        platform = 'arduino'
    py_dir = os.path.dirname(os.path.abspath(__file__))
    tmpl = py_dir + '/templates'
    root = to + '/' + name
    
    os.mkdir(root)

    globals()['gen_for_' + platform](tmpl, thingmodel, root)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='ticos thingmodel generator')
    parser.add_argument('--platform', type=str, help='supported platform: arduino (default) | esp32')
    parser.add_argument('--thingmodel', type=str, help='json file|data of thing model')
    parser.add_argument('--to', type=str, default='.', help='target directory')
    args = parser.parse_args()
    generate(DST_DIR, args.platform, args.thingmodel, args.to)
