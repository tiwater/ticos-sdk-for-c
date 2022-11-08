import requests
import sys
import logging
import json

logging.basicConfig(level=logging.INFO, stream=sys.stdout)
logger = logging.getLogger()
logger.setLevel(level=logging.INFO)

url = "https://api.ticos.cn/products"


def get_product(productId='', token=''):
    logger.info(token)
    params = {"productId": productId}
    res = requests.get(url=url+'/'+productId+'/deviceModel', params=params, headers={
                       'Authorization': token})

    resjson = json.loads(res.text)
    logger.info(resjson)
    logger.info(resjson['deviceModel'])
    return resjson['deviceModel']
    # with open('/tmp/')
