for i in $( seq 0 3 ); do
    convert beat${i}.png -alpha set -background none -channel A -evaluate multiply 0.5 +channel debris${i}.png
done
