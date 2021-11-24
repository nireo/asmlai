if [ ! -f ../asmlai ]
then (cd ..; cmake--build build/; mv build/asmlai ./)
fi

for i in test*
do if [ ! -f "out.$i" -a ! -f "err.$i" ]
   then echo "didn't find output for given test."

   else if [ -f "out.$i" ]
        then
          echo -n $i
          ../asmlai $i

          gcc -o out out.s ../lib/printint.c
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
