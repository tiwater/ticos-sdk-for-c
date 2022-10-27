# coding=utf-8
import os, sys, re, argparse

from datetime import datetime
from string import Template

TELE = 'telemetry'
PROP = 'property'
CMMD = 'command'

NAME = 'name'
TYPE = '@type'
SCHEMA = 'schema'

def type_to_iot_val_enum(t):
    ''' 根据实际数据类型返回对应的TICOS_VAL_TYPE '''
    return 'TICOS_VAL_TYPE_' + t.upper()

def type_to_iot_val_type(t):
    ''' 根据实际数据类型返回对应的c语言类型 '''
    if t == 'boolean':
        return 'bool'
    elif t == 'integer':
        return 'int'
    elif t == 'float':
        return 'float'
    elif t == 'string':
        return 'const char*'
    elif t == 'enum':
        return 'int'
    elif t == 'timestamp':
        return 'time_t'
    elif t == 'duration':
        return 'time_t'
    return t

def gen_func_name_getter(_key, _id):
    return ' ticos_' + _key + '_' + _id + '_send'

def gen_func_name_setter(_key, _id):
    return 'ticos_' + _key + '_' + _id + '_recv'

def gen_func_head_getter(_key, _id, _type):
    return '\n' + _type + gen_func_name_getter(_key, _id) + '(void)'

def gen_func_head_setter(_key, _id, _type):
    return '\nint ' + gen_func_name_setter(_key, _id) + '(' + _type + ' ' + _id + '_)'

def gen_func_body_getter(_key, _id, _type):
    ''' 根据物模型json内容返回对应的getter函数内容 '''
    body = '\n    return 0;'
    return ' {' + body + '\n}\n'

def gen_func_body_setter(_key, _id, _type):
    ''' 根据物模型json内容返回对应的setter函数内容 '''
    body = ''
    return ' {' + body + '\n    return 0;\n}\n'

def gen_func_decs(item, need_getter, need_setter):
    ''' 根据物模型json内容返回对应的函数声明 '''
    decs = ''
    _k = item[TYPE]
    _i = item[NAME]
    _t = type_to_iot_val_type(item[SCHEMA])
    if need_getter:
        decs += gen_func_head_getter(_k, _i, _t) + ';'
    if need_setter:
        decs += gen_func_head_setter(_k, _i, _t) + ';'
    return decs

def gen_func_defs(item, need_getter, need_setter):
    ''' 根据物模型json内容返回对应的函数定义 '''
    defs = ''
    _k = item[TYPE]
    _i = item[NAME]
    _t = type_to_iot_val_type(item[SCHEMA])
    if need_getter:
        head = gen_func_head_getter(_k, _i, _t)
        defs += head + gen_func_body_getter(_k, _i, _t)
    if need_setter:
        head = gen_func_head_setter(_k, _i, _t)
        defs += head + gen_func_body_setter(_k, _i, _t)
    return defs

def gen_table(item, need_getter, need_setter):
    ''' 根据物模型json内容返回对应的方法表成员注册 '''
    _k = item[TYPE]
    _i = item[NAME]
    _t = item[SCHEMA]
    _e = type_to_iot_val_enum(_t)
    getter = gen_func_name_getter(_k, _i)
    setter = gen_func_name_setter(_k, _i)
    if need_getter:
        if need_setter:
            return '\n    { \"%s\", %s, %s, %s },' %(_i, _e, getter, setter)
        else:
            return '\n    { \"%s\", %s, %s },' %(_i, _e, getter)
    else:
        return '\n    { \"%s\", %s, %s },' %(_i, _e, setter)

def gen_enum(item):
    ''' 根据物模型json文件返回对应的物模型枚举列表 '''
    _k = item[TYPE].upper()
    _i = item[NAME]
    return '\n    TICOS_' + _k + '_' + _i + ','

def gen_public_vars(item):
    _k = item[TYPE]
    _i = item[NAME]
    _t = type_to_iot_val_type(item[SCHEMA])
    return  _t + ' ' + _k + '_' + _i + ';'

def gen_iot(date, tmpl_dir, json_file):
    ''' 根据物模型json文件返回对应的物模型接口文件 '''
    import json

    raw = None
    with open(json_file, 'r', encoding='utf-8') as f:
        raw = json.load(f)

    if not raw:
        raise Exception('物模型文件异常')

    func_decs = ''
    tele_enum = ''
    prop_enum = ''
    cmmd_enum = ''

    func_defs = ''
    tele_tabs = ''
    prop_tabs = ''
    cmmd_tabs = ''

    for item in raw[0]['contents']:
        item[TYPE] = item[TYPE].lower()
        _type = item[TYPE]
        if _type == TELE:
            func_decs += gen_func_decs(item, True, False)
            func_defs += gen_func_defs(item, True, False)
            tele_tabs += gen_table(item, True, False)
            tele_enum += gen_enum(item)
        elif _type == PROP:
            func_decs += gen_func_decs(item, True, True)
            func_defs += gen_func_defs(item, True, True)
            prop_tabs += gen_table(item, True, True)
            prop_enum += gen_enum(item)
        elif _type == CMMD:
            func_decs += gen_func_decs(item, False, True)
            func_defs += gen_func_defs(item, False, True)
            cmmd_tabs += gen_table(item, False, True)
            cmmd_enum += gen_enum(item)
    tele_enum += gen_enum({ TYPE:TELE, NAME:'MAX'}) + '\n'
    prop_enum += gen_enum({ TYPE:PROP, NAME:'MAX'}) + '\n'
    cmmd_enum += gen_enum({ TYPE:CMMD, NAME:'MAX'}) + '\n'

    dot_c_lines = []
    with open(tmpl_dir + 'iot_c', 'r', encoding='utf-8') as f:
        tmpl = Template(f.read())
        dot_c_lines.append(tmpl.substitute(
                    GEN_DATE = date,
                    FUNC_DEFS = func_defs,
                    TELEMETRY_TABS = tele_tabs,
                    PROPERTY_TABS = prop_tabs,
                    COMMAND_TABS = cmmd_tabs))
    with open('ticos_thingmodel.c', 'w', encoding='utf-8') as f:
        f.writelines(dot_c_lines)

    dot_h_lines = []
    with open(tmpl_dir + 'iot_h', 'r', encoding='utf-8') as f:
        tmpl = Template(f.read())
        dot_h_lines.append(tmpl.substitute(
                    GEN_DATE = date,
                    FUNC_DECS = func_decs,
                    TELEMETRY_ENUM = tele_enum,
                    PROPERTY_ENUM = prop_enum,
                    COMMAND_ENUM = cmmd_enum))

    with open('ticos_thingmodel.h', 'w', encoding='utf-8') as f:
        f.writelines(dot_h_lines)

    wrapper_c_lines = []
    with open(tmpl_dir + 'mqtt_wrapper_c', 'r', encoding='utf-8') as f:
        tmpl = Template(f.read())
        wrapper_c_lines.append(tmpl.substitute())

    with open('ticos_mqtt_wrapper.c', 'w', encoding='utf-8') as f:
        f.writelines(wrapper_c_lines)

def generate(json_file=''):
    datetime_mark = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    py_dir = os.path.dirname(os.path.abspath(__file__))
    tmpl_dir = py_dir + '/templates/'

    if not json_file:
        raise Exception('请指定物模型json文件')
    gen_iot(datetime_mark, tmpl_dir, json_file)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='ticos_thingmodel_gen')
    parser.add_argument('--json', type=str, default='', help='iot json file')
    args = parser.parse_args()
    generate(args.json)
