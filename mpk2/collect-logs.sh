ls summary-* | while read f; do echo -n $f: && tail -1 $f ; done 
