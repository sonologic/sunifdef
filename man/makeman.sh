rel_month='JANUARY 2008'
sed -e "s/{{REL_MONTH}}/$rel_month/" < man_pod.pl > man_pod_rel.tmp
pod2man man_pod_rel.tmp --section=1 --name=sunifdef --center="User Commmands" --date="strudl.org" --release="$rel_month" > sunifdef.1
pod2html --noindex --title="Sunifdef Man Page" --infile=man_pod_rel.tmp --outfile=html/sunifdef_man_1.html
rm -f *.tmp
