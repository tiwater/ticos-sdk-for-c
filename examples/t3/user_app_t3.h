#ifndef __TICOS_USER_APP_H
#define __TICOS_USER_APP_H
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct ticos_skin_res{
    //char acid[40];
    //char userid[40];
    int skinscore;
    //int monthtimes;
    //char imageurl[200];
    //char reportUrl[200];
    int status;         // 1. 检测中.  2 检测完成            3 检测失败.
    //char createdtime[40];
    char summary[200];
    //char productid[20];
    //char devicename[20];
}ticos_skin_res_t;
void ticos_device_bind_cb(int bind_result);
ticos_skin_res_t *ticos_get_skin_res(void);
#ifdef __cplusplus
}
#endif
#endif

