# LLM-POK

llama2.c + Stories + POK + EMA

Main code:
https://github.com/SergeyStaroletov/LLM-POK/blob/main/examples/llm_demo/pr1/activity.c 

To build and run: 

./init.sh

make configure

unzip big .h from examples/llm_demo/pr1/model_weights.h.zip 

cd examples/LLM-POK/examples/llm_demo

make

make -C . run

....

sudo killall qemu-system-i386


Tested only with qemu-system-i386 on Mac
