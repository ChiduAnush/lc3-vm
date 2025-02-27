This is a implementation of an LC3 virtual machine in C, allowing you to run LC3 `.obj` files compiled from LC3 assembly.

---

### how to use it yourself

#### necessary
you need a tool called `lc3as` to assemble the LC3 programs.
use [this](https://github.com/dennis97519/homebrew-lc3tools)repo to get the lc3tools for macbook using homebrew.
once you get that done, you can use the `lc3as <path to .asm file>` command.
Although i have provided the needed `.obj` file in the `examples` folder, the `lc3as` command is still usefull if you want to write you own lc3 assembly code.

#### running the vm
`git clone <https://github.com/ChiduAnush/lc3-vm>`

You can either run it using cmake(easier), or you can run it manually yourself.

##### Using CMake:
I have already provided the `.obj` file so you dont need the `lc3as` command.

1. after cloning the repo, make sure you are in the root folder.
   `cd lc3-vm`
2. Build the project with Cmake:
   ```
   mkdir build && cd build
   cmake ..
   make
   ```

   This will:
   Create a `build/` folder
   Generate necessary build files
   Compile the VM (`chiduslc3vm` executable)

3. Run it with a .obj file:
   `./chiduslc3vm ../examples/checkKeyboardinput.obj`



##### Doing it manually: 
follow below steps to test the vm using the provided asm file.

1. cd to examples folder. 
   `cd examples`
2. assemble the provided asm file.
   `lc3as checkKeyboardinput.asm`
   This will create 2 new files, `checkKeyboardinput.obj` & `checkKeyboardinput.sym`
3. cd to src folder.
   `cd ../src`
4. compile the vm.
   `gcc-14 vm.c`
   I am using gcc-14. you can use any c compiler.
5. run the vm with the .`obj` file as input.
   `./a.out ../examples/checkKeyboardinput.obj `
6. Congrats! if your program works, pat yourself on the back. if not, well, i am happy you tried lol.(keep trying!)


You can download some other `.obj` files too, and run it in this vm.
such as [2048](https://github.com/rpendleton/lc3-2048) or my [Roguelike](https://github.com/justinmeiners/lc3-rogue).
