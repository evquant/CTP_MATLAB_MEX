if(isconnect)
    error('CTP已经连接，请断开连接再编译');
end
% 编译项目
frameworkpath = fullfile(cd, 'CTP_FRAMEWORK');

mex(['-I', frameworkpath], ... 
    fullfile(['-I', frameworkpath], 'ThostTraderApi'), ...
    fullfile(['-L', frameworkpath], 'ThostTraderApi'), ...
    '-lthostmduserapi.lib', ...
    '-lthosttraderapi.lib', ...
    'TraderMain.cpp', ...
    'CTP_FRAMEWORK\TraderApi.cpp', ...
    'CTP_FRAMEWORK\toolkit.cpp', ...
    'CTP_FRAMEWORK\MdUserApi.cpp', ...
    'CTP_FRAMEWORK\FunctionCallBackSet.cpp', ...
    'CTP_FRAMEWORK\CTPMsgQueue.cpp');
