# Pipes, redirections, logic operators
echo hello | tr a-z A-Z
echo first > /tmp/gb_demo.txt
echo second >> /tmp/gb_demo.txt
cat < /tmp/gb_demo.txt
true && echo "and: ok"
false || echo "or: ok"
(echo group1; echo group2) | cat
exit
