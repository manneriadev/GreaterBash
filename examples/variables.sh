# Variables and special parameters
name=world
echo "hello, $name"
echo 'single quotes: $name'
set COUNT 5
echo $COUNT
export GB_DEMO=exported
echo $GB_DEMO
false
echo "last exit code: $?"
echo "shell pid: $$"
unset name
echo "[$name]"
exit
