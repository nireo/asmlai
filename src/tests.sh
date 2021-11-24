for i in input*
do if [ ! -f "out.$i" -a ! -f "err.$i" ]
   then echo "Can't run test on $i, no output file!"

   else if [ -f "out.$i" ]
        then
          echo -n $i
          ../asmlai $i

          cc -o out out.s ../src/libc/print_num.c
          ./out > trial.$i

          cmp -s "out.$i" "trial.$i"

          if [ "$?" -eq "1" ]
          then echo ": failed"
            diff -c "out.$i" "trial.$i"
            echo

          else echo ": OK"
          fi

   else if [ -f "err.$i" ]
        then
          echo -n $i
          ../comp1 $i 2> "trial.$i"
          cmp -s "err.$i" "trial.$i"
          if [ "$?" -eq "1" ]
          then echo ": failed"
            diff -c "err.$i" "trial.$i"
            echo
          else echo ": OK"
          fi
        fi
     fi
   fi
   rm -f out out.s "trial.$i"
done
