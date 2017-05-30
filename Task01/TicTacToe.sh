#!/bin/bash

# Кулаков Владислав КБ-401

function readkeys()
{
#   Чтение клавиатуры
    tempkey="-1,-1,-1"
    read -s -t 0.05 -n 1 key[0]
    if [ "$?" -ne 0 ]; then
        tempkey="-1,-1,-1"
    else
        read -s -t 0.05 -n 1 key[1]
        if [ "$?" -ne 0 ]; then
            tempkey=`printf "%d,-1,-1" "'${key[0]}"`
        else
            read -s -t 0.05 -n 1 key[2]
            if [ "$?" -ne 0 ]; then
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

function pulsar()
{
#   Скидывает nc
    while true
    do
        sleep 0.1
        echo "0" | nc -U $socket >/dev/null 2>&1
    done
}

function end()
{
#   Завершение скрипта
    if [ "$keycode" -eq 1 ]; then
        if [ "$curside" == "$side" ]; then
            temp=`fieldtostr`
            gpid=0
            while [ "$gpid" != "$opponentpid" ]
            do
                gpid=`echo $xpid"!"$opid"!"$temp"!""E" | nc -Ul $socket`
            done
        elif [ "$side" != "" ]; then
            gmsg=""
            while [ "$gmsg" == "" ]
            do
                gmsg=`echo "65536" | nc -U $socket 2>/dev/null`
            done
        fi
    fi
    if [ "$pulsarpid" -ne 0 ]; then
        disown $pulsarpid
        kill $pulsarpid
    fi
    draw $(($ydown+2)) 0
    stty $rstty
    exit
}

function draw()
{
#   Отрисовка в нужной точке
    tput cup $1 $2
    echo -ne "$3"
}

function setplaces()
{
#   Отобразить поле
    x=$xf
    y=$yf
    for i in "${field[@]}"
    do
        tput cup $y $x
        if [ "$i" == "Z" ]
        then
            echo -n " "
        else
            echo -n $i
        fi
        x=$(($x+2))
        if [ "$x" -eq "$(($xf+6))" ]
        then
            y=$(($y+2))
            x=$xf
        fi
    done
}

function waiter()
{
#   Ожидание действия оппонента
    if [ "$tmt" -eq 0 ]; then
        if [ "$k" == "..." ]; then
            k="."
        else
            k=$k"."
        fi
    fi
    tmt=$((($tmt+1)%10))
    if [ "$tp" != "Spectator" ]; then
        draw $(($yup+$1)) $((xup+13)) "Wait for opponent    "
        draw $(($yup+$1)) $((xup+13)) "Wait for opponent "$k
    else
        draw $(($yup+$1)) $((xup+13)) "Wait for move        "
        draw $(($yup+$1)) $((xup+13)) "Wait for move "$k
    fi
    draw $(($ydown+2)) 0
}

function fieldtostr()
{
    s=""
    for i in ${field[@]}
    do
        s=$s$i
    done
    echo $s
}

function strtofield()
{
    j=0
    for i in `echo $1 | sed "s/./&\n/g"`
    do
        field[$j]=$i
        j=$(($j+1))
    done
}

function getpacket()
{
#   Разбор пакета [(pid X)!(pid O)!(поле)!(чей ход)]
    gmsg=`echo $$ | nc -U $socket 2>/dev/null`
    if [ "$gmsg" == "" ]; then
        return
    fi
    gxpid=`echo $gmsg | sed -sr "s/([0-9]+)!.*/\1/g"`
    gopid=`echo $gmsg | sed -sr "s/[0-9]+!([0-9]+)!.*/\1/g"`
    gfield=`echo $gmsg | sed -sr "s/[0-9]+![0-9]+!(.*)!./\1/g"`
    gcurside=`echo $gmsg | sed -sr "s/[0-9]+![0-9]+!.*!(.)/\1/g"`
}

function manual()
{
#   Управление
    draw $(($yup+9)) $(($xup+12))  "<Key Up>    - to go up"
    draw $(($yup+10)) $(($xup+12)) "<Key Down>  - to go down"
    draw $(($yup+11)) $(($xup+12)) "<Key Left>  - to go left"
    draw $(($yup+12)) $(($xup+12)) "<Key Right> - to go right"
    draw $(($yup+13)) $(($xup+12)) "<Key Esc>   - to go quit"
}

function checkgame()
{
#   Проверка на победу
    for i in `echo $win`
    do
        a=${field[${i:0:1}]}
        b=${field[${i:1:1}]}
        c=${field[${i:2:1}]}
        if [ "$a" == "$b" ]; then
            if [ "$b" == "$c" ]; then
                if [ "$a" != "Z" ]; then
                    winside=$a
                    return
                fi
            fi
        fi
    done
#   Проверка на ничью
    winside="D"
    for i in ${field[@]}
    do
        if [ "$i" == "Z" ]; then
            winside="Z"
            return
        fi
    done
}

function chooseplace()
{
#   процесс хода
    case "$keycode" in
        "2" )
            if [ "$place" -gt 2 ]; then
                place=$(($place-3))
            fi
        ;;
        "3" )
            if [ "$place" -lt 6 ]; then
                place=$(($place+3))
            fi
        ;;
        "4" )
            if [ "$(($place%3))" -ne 2 ]; then
                place=$(($place+1))
            fi
        ;;
        "5" )
            if [ "$(($place%3))" -ne 0 ]; then
                place=$(($place-1))
            fi
        ;;
        "6" )
            if [ "${field[$place]}" == "Z" ]; then
                field[$place]=$side
                setplaces
                if [ "$curside" == "X" ]; then
                    newcurside="O"
                else
                    newcurside="X"
                fi
            fi
        ;;
        * )
        ;;
    esac
    tput cup $(($yf+2*($place/3))) $(($xf+2*($place%3)))
}

function spectator()
{
    draw $(($yup+9)) $(($xup+12))  "                        "
    draw $(($yup+10)) $(($xup+12)) "                        "
    draw $(($yup+11)) $(($xup+12)) "                        "
    draw $(($yup+12)) $(($xup+12)) "                         "
    draw $(($yup+7)) $((xup+13)) "                     "
    draw $(($yup+1)) $(($xup+17)) "Player1"
    draw $(($yup+2)) $(($xup+17)) "PID: $xpid"
    draw $(($yup+3)) $(($xup+17)) "Side: X"
    draw $(($yup+5)) $(($xup+17)) "Player2"
    draw $(($yup+6)) $(($xup+17)) "PID: $opid"
    draw $(($yup+7)) $(($xup+17)) "Side: O"
    while true
    do
        readkeys
        if [ "$keycode" -eq 1 ]; then
            keycode=0
            end
        fi
        getpacket
        if [ "$gmsg" == "" ]; then
            continue
        fi
        if [ "$curside" != "$gcurside" ]; then
            strtofield $gfield
            curside=$gcurside
            setplaces
            checkgame
            if [ "$winside" != "Z" ]; then
                break
            fi
        fi
        waiter 9
    done
    draw $(($yup+9)) $((xup+13)) "                     "
    if [ "$winside" == "X" ]; then
        draw $(($yup+9)) $((xup+17)) "Player1 Win!"
    elif [ "$winside" == "O" ]; then
        draw $(($yup+9)) $((xup+17)) "Player2 Win!"
    else
        draw $(($yup+9)) $((xup+17)) "Draw!"
    fi
}

function game()
{
#   Основной процесс
    draw $(($yup+7)) $((xup+13)) "                     "
    if [ "$tp" == "Server" ]; then
        draw $(($yup+1)) $(($xup+16)) "Op: Client"
    else
        draw $(($yup+1)) $(($xup+16)) "Op: Server"
    fi
    draw $(($yup+2)) $(($xup+16)) "PID: "$opponentpid
    if [ "$side" == "X" ]; then
        draw $(($yup+3)) $(($xup+16)) "Side: O"
    else
        draw $(($yup+3)) $(($xup+16)) "Side: X"
    fi
    draw $(($yup+5)) $(($xup+16)) "Current Side: "$curside
    newcurside=$curside
    while true
    do
        readkeys
        if [ "$keycode" -eq 1 ]; then
            end
        fi
        if [ "$curside" == "$side" ]; then
            temp=`fieldtostr`
            gpid=`echo $xpid"!"$opid"!"$temp"!"$newcurside | nc -Ul $socket`
            if [ "$gpid" == "65536" ]; then
                end
            fi
            if [ "$newcurside" != "$curside" ]; then
                if [ "$gpid" == "$opponentpid" ]; then
                    curside=$newcurside
                    draw $(($yup+5)) $(($xup+16)) "Current Side: "$curside
                    checkgame
                    if [ "$winside" != "Z" ]; then
                        break
                    fi
                fi
            else
                chooseplace
            fi
        else
            waiter 7
            getpacket
            if [ "$gmsg" == "" ]; then
                continue
            fi
            if [ "$gcurside" == "E" ]; then
                end
            fi
            if [ "$curside" != "$gcurside" ]; then
                curside=$gcurside
                draw $(($yup+5)) $(($xup+16)) "Current Side: "$curside
                newcurside=$curside
                strtofield $gfield
                setplaces
                place=0
                draw $(($yup+7)) $((xup+13)) "                     "
                checkgame
                if [ "$winside" != "Z" ]; then
                    break
                fi
            fi
        fi
        keycode=0
    done
    if [ "$winside" == "$side" ]; then
        draw $(($yup+7)) $((xup+16)) "You Win!"
    elif [ "$winside" != "D" ]; then
        draw $(($yup+7)) $((xup+16)) "You Lose!"
    else
        draw $(($yup+7)) $((xup+16)) "Draw!"
    fi
    if [ "$pulsarpid" -ne 0 ]; then
        while true
        do
            if [ "$keycode" -eq 1 ]; then
                keycode=0
                break
            fi
            temp=`fieldtostr`
            gpid=`echo $xpid"!"$opid"!"$temp"!"$curside | nc -Ul $socket`
            readkeys
        done
    fi
}

function chooseside()
{
#   Выбор стороны
    draw $(($yup+6)) $((xup+15)) "Choose your side"
    draw $(($yup+8)) $((xup+19)) "X"
    draw $(($yup+8)) $((xup+26)) "O"
    side="X"
    tput cup $(($yup+8)) $((xup+19))
    while true
    do
        readkeys
        case "$keycode" in
            "1" )
                end
            ;;
            "4" )
                side="O"
            ;;
            "5" )
                side="X"
            ;;
            "6" )
                break
            ;;
            * )
                continue
            ;;
        esac
        if [ "$side" == "X" ]; then
            tput cup $(($yup+8)) $((xup+19))
        else
            tput cup $(($yup+8)) $((xup+26))
        fi
        keycode=0
    done
    draw $(($yup+6)) $((xup+15)) "                "
    draw $(($yup+8)) $((xup+15)) "                "
    draw $(($yup+3)) $(($xup+2)) "Side: "$side
    draw $(($ydown+2)) 0
}

function server()
{
    tp="Server"
    draw $(($yup+1)) $(($xup+2)) "Me: "$tp
    pulsar &
    pulsarpid=$!
    chooseside
    if [ "$side" == "X" ]; then
        xpid=$$
    else
        opid=$$
    fi
    temp=`fieldtostr`
    k="."
    tmt=0
    while true
    do
        waiter 7
        readkeys
        if [ "$keycode" -eq 1 ]; then
            end
        fi
        keycode=0
        gpid=`echo $xpid"!"$opid"!"$temp"!"$curside | nc -Ul $socket`
        if [ "$gpid" -eq 0 ]; then
            continue
        else
            if [ "$side" == "X" ]; then
                opid=$gpid
                opponentpid=$gpid
            else
                xpid=$gpid
                opponentpid=$gpid
            fi
            break
        fi
    done
    game
}

function client()
{
    tp="Client"
    manual
    while true
    do
        waiter 7
        readkeys
        if [ "$keycode" -eq 1 ]; then
            end
        fi
        keycode=0
        getpacket
        if [ "$gmsg" == "" ]; then
            continue
        fi
        if [ "$gxpid" -eq 0 ]; then
            if [ "$gopid" -eq 0 ]; then
                continue
            else
                side="X"
                gxpid=$$
                opponentpid=$gopid
            fi
        else
            if [ "$gopid" -eq 0 ]; then
                side="O"
                opid=$$
                opponentpid=$gxpid
            else
                tp="Spectator"
                side="Z"
            fi
        fi
        xpid=$gxpid
        opid=$gopid
        strtofield $gfield
        curside=$gcurside
        break
    done
    draw $(($yup+1)) $(($xup+2)) "Me: "$tp
    draw $(($yup+3)) $(($xup+2)) "Side: "$side
    if [ "$tp" == "Spectator" ]; then
        spectator
    else
        game
    fi
}

function show()
{
#   Первичная отрисовка интерфейса
    draw $yup $xup "\u250F"
    draw $ydown $xup "\u2517"
    draw $yup $xdown "\u2513"
    draw $ydown $xdown "\u251B"
    for i in `seq $(($xup+1)) $(($xdown-1))`
    do
        draw $yup $i "\u2501"
        draw $ydown $i "\u2501"
    done
    for i in `seq $(($yup+1)) $(($ydown-1))`
    do
        draw $i $xup "\u2503"
        draw $i $xdown "\u2503"
    done
    draw $(($yup+2)) $(($xup+2)) "PID: "$$
    draw $(($yf-1)) $(($xf-1)) "\u250C\u2500\u252C\u2500\u252C\u2500\u2510"
    draw $(($yf+0)) $(($xf-1)) "\u2502 \u2502 \u2502 \u2502"
    draw $(($yf+1)) $(($xf-1)) "\u251C\u2500\u253C\u2500\u253C\u2500\u2524"
    draw $(($yf+2)) $(($xf-1)) "\u2502 \u2502 \u2502 \u2502"
    draw $(($yf+3)) $(($xf-1)) "\u251C\u2500\u253C\u2500\u253C\u2500\u2524"
    draw $(($yf+4)) $(($xf-1)) "\u2502 \u2502 \u2502 \u2502"
    draw $(($yf+5)) $(($xf-1)) "\u2514\u2500\u2534\u2500\u2534\u2500\u2518"
}

function main()
{
#   Начало...
    stty -echo
    echo -e "\ec"
    show
    temp=`ps -xa | grep "./TicTacToe.sh" | wc -l`
    if [ "$temp" -lt 4 ]; then
#       Первый запущенный
        server
    else
        client
    fi
    end
}

#   --- Переменные ---

# сохраняем флаги терминала
rstty=`stty -g`
# Unix сокет
socket="/tmp/tictactoe.socket"
# pid дополнительных процессов
opponentpid=0
pulsarpid=0
# кординаты рамки
xup=3
yup=1
xdown=41
ydown=15
# кординаты первой клетки
xf=$(($xup+5))
yf=$(($yup+6))
# тип клиента
tp=""
# данные пакета
xpid=0
opid=0
strtofield "ZZZZZZZZZ"
curside="X"
gxpid=0
gopid=0
# победные
win="012 036 048 147 246 258 345 678"
winside="Z"
# остальные
k="."
tmt=0
place=0
#   ------------------

main
