/*****************************************************************************
 * File name: TraderMain.cpp
 * Description: CTP for matlab mex. Designed a series of operations to operate the CTP
 * Author: jebin
 * Version: 1.0
 * Date: 2014/07/10
 * History: see git log
 *****************************************************************************/

#include "Connection.h"
#include "mxStructTool.h"

#include "mex.h"
#include "matrix.h"

#include<iostream>
#include<cstring>
#include<string>
#include<set>
#include<utility>

using namespace std;

//连接总参数
Connection *Con;

void CheckIsConnect()
{
    if(NULL == Con)
        mexErrMsgTxt("未连接CTP!");
}

//mex主函数入口
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, mxArray *prhs[])
{
    //为保证速度，未添加安全判断
    int choise = (int)mxGetScalar(prhs[0]);
    switch(choise)
    {
        
        //连接CTP
        case 1:
        {
            if(NULL == Con)
            {
                Con = new Connection();
                CheckIsConnect();
                Con->readInifile(".\\server.ini", mxArrayToString(prhs[1]));
                Con->td->Connect(Con->streamPath,
                        Con->tdServer,
                        Con->brokerId,
                        Con->investorId,
                        Con->password,
                        THOST_TERT_RESTART, "", "");
                WaitForSingleObject(Con->callbackSet->h_connected, 5000);
                if(Con->callbackSet->bIsTdConnected == false)
                {
                    delete Con;
                    Con = NULL;
                    mexPrintf("交易端连接失败\n");
                }
                else
                {
                    mexPrintf("交易端连接成功\n");
                    Con->md->Connect(Con->streamPath,
                            Con->mdServer,
                            Con->brokerId,
                            Con->investorId,
                            Con->password);
                    WaitForSingleObject(Con->callbackSet->h_connected, 5000);
                    if(Con->callbackSet->bIsMdConnected == false)
                    {
                        delete Con;
                        Con = NULL;
                        mexPrintf("行情端连接失败\n");
                    }
                    else
                    {
                        mexPrintf("行情端连接成功\n");
                        plhs[0] = mxCreateDoubleScalar(Con->td->m_RspUserLogin.FrontID);
                        plhs[1] = mxCreateDoubleScalar(Con->td->m_RspUserLogin.SessionID);
                        Con->td->ReqQryInstrument("");
                        mexPrintf("获取合约成功\n");
                    }
                }
            }
            else
            {
                mexWarnMsgTxt("已经连接!");
                plhs[0] = mxCreateDoubleScalar(Con->td->m_RspUserLogin.FrontID);
                plhs[1] = mxCreateDoubleScalar(Con->td->m_RspUserLogin.SessionID);
            }
            break;
        }
        
        //断开CTP
        case 2:
        {
            CheckIsConnect();
            delete Con;
            Con = NULL;
            mexPrintf("断开连接成功\n");
            break;
        }
        
        //订阅行情
        case 3:
        {
            CheckIsConnect();
            string inst = mxArrayToString(prhs[1]);
            if(inst.size() == 0)
            {
                if(Con->callbackSet->bIsGetInst)
                {
                    Con->md->Subscribe(Con->callbackSet->strAllIns);
                    mexPrintf("订阅完成\n");
                }
                else
                    mexWarnMsgTxt("未查询所有合约，不能使用订阅全部合约");
            }
            else
            {
                Con->md->Subscribe(inst);
                mexPrintf("订阅完成\n");
            }
            break;
        }
        
//         查询合约信息
        case 4:
        {
            CheckIsConnect();
            plhs[0] = GetInstInfo(Con->callbackSet->GetInstInfo());
            break;
        }
        
//         获取合约当前所有行情数据信息
        case 5:
        {
            CheckIsConnect();
            if(nrhs == 1)
                plhs[0] = GetMarketData(Con->md->GetSubInst(), Con->callbackSet->GetAllMarketData());
            else
            {
                string inst = mxArrayToString(prhs[1]);
                plhs[0] = GetMarketData(Con->callbackSet->GetMarketData(inst));
            }
            break;
        }
        
        //下单操作
        case 6:
        {
            CheckIsConnect();
            map<pair<int, pair<int, string> >, CThostFtdcOrderField> &orders = Con->callbackSet->m_orders;
            string inst = mxArrayToString(prhs[1]);
            string direction = mxArrayToString(prhs[2]);
            string offsetFlag = mxArrayToString(prhs[3]);
            double volume = mxGetScalar(prhs[4]);
            double price = mxGetScalar(prhs[5]);
            int ref = Con->td->ReqOrderInsert(inst, direction[0], offsetFlag.c_str(), "1", (int)volume, price,
                    THOST_FTDC_OPT_LimitPrice, THOST_FTDC_TC_GFD, THOST_FTDC_CC_Immediately,
                    0, THOST_FTDC_VC_AV);
            char buf[105];
            itoa (ref,buf,10);
            pair<int, pair<int, string> > order =
                    make_pair(Con->td->m_RspUserLogin.FrontID, make_pair(Con->td->m_RspUserLogin.SessionID, string(buf)));
            int timeout = 0;
            while(orders.find(order) == orders.end())
            {
                ++timeout;
                Sleep(1);
                if(timeout > 3000)
                    break;
            }
            if(timeout > 3000)
                plhs[0] = mxCreateDoubleScalar(-1);
            else
                plhs[0] = mxCreateDoubleScalar(ref);
            break;
        }
        
        //获取所有报单信息
        case 7:
        {
            CheckIsConnect();
            map<pair<int, pair<int, string> >, CThostFtdcOrderField> &orders = Con->callbackSet->m_orders;
            //查询所有报单
            if(nrhs == 1)
            {
                pair<int, pair<int, string> > order = make_pair(0, make_pair(0, ""));
                plhs[0] = GetOrderData(Con->callbackSet->GetOrderInfo(), order);
            }
            //查询当前连接报单
            else if(nrhs == 2)
            {
                string ref = mxArrayToString(prhs[1]);
                pair<int, pair<int, string> > order =
                        make_pair(Con->td->m_RspUserLogin.FrontID, make_pair(Con->td->m_RspUserLogin.SessionID, ref));
                if(orders.find(order) != orders.end())
                    plhs[0] = GetOrderData(Con->callbackSet->GetOrderInfo(), order);
                else
                {
                    plhs[0] = mxCreateDoubleScalar(0);
                    mexWarnMsgTxt("未存在此报单\n");
                }
            }
            //查询所有连接中指定报单
            else if(nrhs == 4)
            {
                int frontid = (int)mxGetScalar(prhs[1]);
                int session = (int)mxGetScalar(prhs[2]);
                string ref = mxArrayToString(prhs[3]);
                pair<int, pair<int, string> > order =
                        make_pair(frontid, make_pair(session, ref));
                if(orders.find(order) != orders.end())
                    plhs[0] = GetOrderData(Con->callbackSet->GetOrderInfo(), order);
                else
                {
                    plhs[0] = mxCreateDoubleScalar(0);
                    mexWarnMsgTxt("未存在此报单\n");
                }
            }
            break;
        }
        
        //撤单
        case 8:
        {
            CheckIsConnect();
            map<pair<int, pair<int, string> >, CThostFtdcOrderField> &orders = Con->callbackSet->m_orders;
            if(nrhs == 2)
            {
                if( !mxIsChar(prhs[1]) )
                {
                    CThostFtdcOrderField order;
                    MxToOrder(order, prhs[1]);
                    Con->td->ReqOrderAction(&order);
                }
                else
                {
                    string ref = mxArrayToString(prhs[1]);
                    pair<int, pair<int, string> > order =
                            make_pair(Con->td->m_RspUserLogin.FrontID, make_pair(Con->td->m_RspUserLogin.SessionID, ref));
                    if(orders.find(order) != orders.end())
                        Con->td->ReqOrderAction(&orders[order]);
                    else
                        mexWarnMsgTxt("未存在此报单\n");
                }
            }
            else if(nrhs == 4)
            {
                int frontid = (int)mxGetScalar(prhs[1]);
                int session = (int)mxGetScalar(prhs[2]);
                string ref = mxArrayToString(prhs[3]);
                pair<int, pair<int, string> > order =
                        make_pair(frontid, make_pair(session, ref));
                if(orders.find(order) != orders.end())
                    Con->td->ReqOrderAction(&orders[order]);
                else
                    mexWarnMsgTxt("未存在此报单\n");
            }
            
            break;
        }
        
        //获取所有持仓信息
        case 9:
        {
            CheckIsConnect();
            // 查询所有持仓
            if(nrhs == 1)
                plhs[0] = GetPositionData(Con->callbackSet->GetPosition());
            // 查询指定持仓
            else if(nrhs == 2)
                plhs[0] = GetPositionData(Con->callbackSet->GetPosition(), mxArrayToString(prhs[1]));
            break;
        }
        //判断是否连接
        case 10:
        {
            bool isconnect = !(NULL == Con);
            plhs[0] = mxCreateLogicalScalar(isconnect);
            break;
        }
        
        //查询错误信息
        case 11:
        {
            CheckIsConnect();
            plhs[0] = GetErrorInfo(Con->callbackSet->GetErrorInfo());
            break;
        }
        default:
            mexErrMsgTxt("没有找到相关操作");
            
    }
    
}
