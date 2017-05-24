#!/usr/bin/env sh

pid=$(ps aux|grep http_capture|grep -v grep|awk '{print $2}')

status(){
    if [ $pid>0 ];then
        echo "Http_capture is running... "
        echo "Http_capture count is : " $(ps aux|grep http_capture|grep -v grep|wc -l)
        echo "Http_capture pid is : $pid"
    else
        echo "Http_capture is closed..."
    fi
}

start(){
    echo "Becareful ! Httpcapture control by supervisor, won't use this to start !"
    return 1
    if [ $pid>0 ];then
        echo "Http_capture is still running... "
        echo "Http_capture pid is : "$pid
    fi
    echo "Starting http_capture ..."
    /home/zhihu/tsp/http_capture/http_capture -i bond2 -abBKd    
    if [ $(ps aux|grep http_capture|grep -v grep|awk '{print $2}')>0 ];then
        echo "Http_capture is running... "
    else
        echo "Http_capture start failed..."
    fi
} 

stop(){
    echo "Stoping http_capture ..."
    if [ $pid>0 ];then
        kill -9 $pid
        echo "Http_capture is closed ~!"
    else
        echo "Http_capture has closed ~!"
    fi
}

restart(){
    echo "Becareful ! Httpcapture control by supervisor, won't use this to start !"
    return 1
    echo "Stoping http_capture ..."
    if [ $pid>0 ];then
        kill -9 $pid
        echo "Http_capture is closed ~!"
    else
        echo "Http_capture has closed ~!"
    fi
    sleep 1
    if [ $(ps aux|grep http_capture|grep -v grep|awk '{print $2}')>0 ];then
        echo "Http_capture is still running, pid is $pid"
    fi

    echo "Starting http_capture ..."
    /home/zhihu/tsp/http_capture/http_capture -i bond2 -abBKd    
    if [ $(ps aux|grep http_capture|grep -v grep|awk '{print $2}')>0 ];then
        echo "Http_capture is running... "
    else
        echo "Http_capture start failed..."
    fi
}

if [ "$1" == "status" ];then
    status
elif [ "$1" == "stop" ];then
    stop
elif [ "$1" == "start" ];then
    start
elif [ "$1" == "restart" ];then
    restart
else
    echo "Invalid command"
fi
