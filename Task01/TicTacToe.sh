#!/bin/bash

function readkeys() # Чтение клавиатуры
{
    tempkey="-1,-1,-1"
    read -s -t 0.1 -n 1 key[0]
    if [ "$?" -ne 0 ]
    then
        tempkey="-1,-1,-1"
    else
        read -s -t 0.1 -n 1 key[1]
        if [ "$?" -ne 0 ]
        then
            tempkey=`printf "%d,-1,-1" "'${key[0]}"`
        else
            read -s -t 0.1 -n 1 key[2]
            if [ "$?" -ne 0 ]
            then
                tempkey=`printf "%d,%d,-1" "'${key[0]}" "'${key[1]}"`
            else
                tempkey=`printf "%d,%d,%d" "'${key[0]}" "'${key[1]}" "'${key[2]}"`
            fi
        fi
    fi
    case "$tempkey" in
        "27,-1,-1" )
            keycode=1                                   # 1 - Escape
            ;;
        "27,91,65" )
            keycode=2                                   # 2 - Up Key
            ;;
        "27,91,66" )
            keycode=3                                   # 3 - Down Key
            ;;
        "27,91,67" )
            keycode=4                                   # 4 - Right Key
            ;;
        "27,91,68" )
            keycode=5                                   # 5 - Left Key
            ;;
        "0,-1,-1" )
            keycode=6                                   # 6 - Enter
            ;;
        * )
            keycode=0
            ;;
    esac
}

function init() # Отрисовка интерфейса
{
    cls
    tput cup $ry $rx
    echo -e "\u259B"
    tput cup $ry $(($rx+$lx-1))
    echo -e "\u259C"
    tput cup $(($ry+$ly-1)) $rx
    echo -e "\u2599"
    tput cup $(($ry+$ly-1)) $(($rx+$lx-1))
    echo -e "\u259F"
    for i in `seq 1 $(($lx-2))`;
    do
        tput cup $ry $(($rx+$i))
        echo -e "\u2580"
        tput cup $(($ry+$ly-1)) $(($rx+$i))
        echo -e "\u2584"
    done
    for i in `seq 1 $(($ly-2))`;
    do
        tput cup $(($ry+$i)) $rx
        echo -e "\u258C"
        tput cup $(($ry+$i)) $(($rx+$lx-1))
        echo -e "\u2590"
    done
    x=3
    y=4
    for i in "${field[@]}"
    do
        tput cup $(($ry+$y)) $(($rx+$x))
        y=$y+1
        echo -e $i
    done
    tput cup $(($ry+1)) $(($rx+$x))
    echo $tp
    tput cup $(($ry+2)) $(($rx+$x))
    echo PID=$$
}

function sendserver()
{
    side
    listen
}

function listen()
{
    
}

function side()
{
    tput cup $(($ry+4)) $(($rx+13))
    echo "What side you"
    tput cup $(($ry+5)) $(($rx+16))
    echo "prefer?"
    tput cup $(($ry+7)) $(($rx+16))
    echo "X"
    tput cup $(($ry+7)) $(($rx+21))
    echo "O"
    tput cup $(($ry+7)) $(($rx+16))
    curside="X"
    while true
    do
        if [ "$curside" == "X" ]
        then
            tput cup $(($ry+7)) $(($rx+16))
        else
            tput cup $(($ry+7)) $(($rx+21))
        fi
        keycode=0
        while [ "$keycode" == "0" ]
        do
            readkeys
        done
        if [ "$keycode" == "5" ]
        then
            curside="X"
        elif [ "$keycode" == "4" ]
        then
            curside="O"
        elif [ "$keycode" == "6" ]
        then
            break
        fi
    done
}

function server()
{
    tp="Server"
    init
    rm -r /tmp/tictactoe.socket 1>/dev/null 2>/dev/null
    sendserver
    tput cup $(($ry+$ly+1)) 0
    #sendserver | netcat -Ul /tmp/tictactoe.socket | get
}

function send()
{
    while true
    do
        read -s -n 1 -t 1 
    done
}

function get()
{
    while true
    do
        read -s -n 1 -t 1 G
    done
}

function client()
{
    tp="Client"
    send | netcat -U /tmp/tictactoe.socket | get
}

function cls()
{
    echo -e "\ec"
}

function main()
{
    rstty=`stty -g`
    stty -echo
    ss -xa | grep /tmp/tictactoe.socket >/dev/null
    if [ "$?" -ne 0 ]
    then
        server
    else
        client
    fi
    stty $rstty
}

# Переменные))
field[0]="\u250F\u2501\u2533\u2501\u2533\u2501\u2513"
field[1]="\u2503 \u2503 \u2503 \u2503"
field[2]="\u2523\u2501\u254B\u2501\u254B\u2501\u252B"
field[3]=${field[1]}
field[4]=${field[2]}
field[5]=${field[1]}
field[6]="\u2517\u2501\u253B\u2501\u253B\u2501\u251B"
rx=3
ry=1
lx=30
ly=12

main
