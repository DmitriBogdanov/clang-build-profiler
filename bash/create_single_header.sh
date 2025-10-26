# __________________________________ CONTENTS ___________________________________
#
#    Script that merges all the individual modules into a single header.
# _______________________________________________________________________________

source bash/variables.sh

for f in $path_include; do
    [[ "$f" = $path_single_include ]] || { cat -- "$f"; printf "\n\n\n\n\n\n"; }
done > $path_single_include
