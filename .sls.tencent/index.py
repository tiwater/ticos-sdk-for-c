import sys
import logging
import get_product
import json
import random
import os
import zipfile
from kick_off import generate

logging.basicConfig(level=logging.INFO, stream=sys.stdout)
logger = logging.getLogger()
logger.setLevel(level=logging.INFO)

logger.info('Loading function')

def copy_file(sourcepath='./templates', targetpath='/tmp/ticossdk/'):
    if not os.path.isdir(targetpath):
        os.makedirs(targetpath)

    filelist_src = os.listdir(sourcepath)
    for file in filelist_src:
        path = os.path.join(os.path.abspath(sourcepath), file)
        if os.path.isdir(path):
            sub_path = os.path.join(os.path.abspath(targetpath), file)
            if not os.path.exists(sub_path):
                os.mkdir(sub_path)
            copy_file(path, sub_path)
        else:
            with open(path, 'rb') as read_stream:
                contents = read_stream.read()
                sub_path = os.path.join(targetpath, file)
                with open(sub_path, 'wb') as write_stream:
                    write_stream.write(contents)


def zip_file(source_path='', zip_path='', zip_name='ticos_sdk.zip'):
    logger.info('start zip')
    zip_name = os.path.join(zip_path, zip_name)
    zip = zipfile.ZipFile(zip_name, 'w', zipfile.ZIP_DEFLATED)
    for dirpath, dirnames, filenames in os.walk(source_path):
        logger.info(dirpath)
        # logger.info(dirnames)
        logger.info(filenames)
        fpath = dirpath.replace(source_path, '')
        fpath = fpath and fpath + os.sep or ''
        logger.info(fpath)
        for filename in filenames:
            if filename != '.DS_Store':
                logger.info(filename)
                zip.write(os.path.join(dirpath, filename), fpath+filename)

    zip.close()


def main_handler(event, content):
    logger.info('start main_handler')
    if "requestContext" not in event.keys():
        return {"code": 410, "message": "event is not come from api gateway"}

    logger.info(event["requestContext"]["path"])
    logger.info(event["requestContext"]["httpMethod"])
    logger.info(event.get("headerParameters") )
    if event["requestContext"]["path"] == "/sdk" and event["requestContext"]["httpMethod"] == "POST":
        body = event['body']
        body_parsed = json.loads(body)
        logger.info(body_parsed)
        model = get_product.get_product(body_parsed['productId'])
        os_set = body_parsed['system']

        folder_r = random.randint(1, 100)
        zip_path = os.path.join('/tmp/ticossdk/', str(folder_r))
        output_path = os.path.join(zip_path, 'output')
       
        os.makedirs(output_path)

        generate(
            'TicosSDK',os_set, model, output_path)
        zip_file(output_path, zip_path)

        zip_stream = open(os.path.join(
            zip_path, 'ticos_sdk.zip'), 'rb').read()
        logger.info(str(zip_stream, encoding='latin-1'))
        return {'sdk': str(zip_stream, encoding='latin-1')}

    logger.info('end main_handler nothing')
    return {'status': 'failed'}
