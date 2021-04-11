#!/bin/sh

#-----------------------------------------------------------------------
#  Created     : 06.04.21
#  Author      : Alin42
#  Description : Shell script to quikly add signs
#-----------------------------------------------------------------------

sign="//-----------------------------------------------------------------------
//  Created     : `date +%d.%m.%y`
//  Author      : `users`
//  Description : TODO add description
//-----------------------------------------------------------------------

"

# TODO add /* */ support
alin_delete_previous() {
    ans="$1"
    a=$(echo "$ans" | head -1)
    while [ -z "$a" -o "$a" = "`echo "$a" | grep "//.*"`" ]
    do
        ans=$(echo "$ans" | tail -n +2)
        a=$(echo "$ans" | head -1)
    done
    echo "$ans"
}

alin_walk_and_sign() {
    for sub in ./*
    do
        if [ -d "$sub" ]
        then
            cd "$sub"
            echo "checking dir $sub"
            alin_walk_and_sign
            cd ..
        elif [ "$sub" = "`echo "$sub" | grep ".*\.h"`" ]
        then
            echo "adding sign to $sub"
            contents=$(cat $sub)
            contents=$(alin_delete_previous "$contents")
            echo "$sign$contents" > "$sub"
        fi
    done
}

alin_walk_and_sign

unset sign
unset alin_walk_and_sign
unset alin_delete_previous
