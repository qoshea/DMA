
 ╔══════════════════════════════════════════════════════════════════════════════════════════════════════════╗
███████▒▒▒▒▒▒▒███████▒▒▒▒▒▒▒███████▒▒▒▒▒▒▒███████▒▒▒▒▒▒▒███████▒▒▒▒▒▒▒███████▒▒▒▒▒▒▒███████▒▒▒▒▒▒▒███████▒▒▒╝
***VIEW W/AT LEAST 115 COLUMNS TO SEE MY BLOCK AND HEAP “DIAGRAM” AND (MORE IMPORTANTLY) MY ASCII ART!***


███╗   ███╗ █████╗ ██╗     ██╗      ██████╗  ██████╗  ██████╗ ███████╗ █████╗ ██████╗ ███╗   ███╗███████╗
████╗ ████║██╔══██╗██║     ██║     ██╔═══██╗██╔════╝  ██╔══██╗██╔════╝██╔══██╗██╔══██╗████╗ ████║██╔════╝
██╔████╔██║███████║██║     ██║     ██║   ██║██║       ██████╔╝█████╗  ███████║██║  ██║██╔████╔██║█████╗  
██║╚██╔╝██║██╔══██║██║     ██║     ██║   ██║██║       ██╔══██╗██╔══╝  ██╔══██║██║  ██║██║╚██╔╝██║██╔══╝  
██║ ╚═╝ ██║██║  ██║███████╗███████╗╚██████╔╝╚██████╗  ██║  ██║███████╗██║  ██║██████╔╝██║ ╚═╝ ██║███████╗
╚═╝     ╚═╝╚═╝  ╚═╝╚══════╝╚══════╝ ╚═════╝  ╚═════╝  ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚═════╝ ╚═╝     ╚═╝╚══════╝
	    		 		      	       	__                              __              
	 			       	               / /_  __  __   ____ _____  _____/ /_  ___  ____ _
	  	    	 	                      / __ \/ / / /  / __ `/ __ \/ ___/ __ \/ _ \/ __ `/
	 			 	             / /_/ / /_/ /  / /_/ / /_/ (__  ) / / /  __/ /_/ / 
				 		    /_.___/\__, /   \__, /\____/____/_/ /_/\___/\__,_/  
    		 	    	 		          /____/      /_/                               
███████▒▒▒▒▒▒▒███████▒▒▒▒▒▒▒███████▒▒▒▒▒▒▒███████▒▒▒▒▒▒▒███████▒▒▒▒▒▒▒███████▒▒▒▒▒▒▒███████▒▒▒▒▒▒▒███████▒▒▒╗  
 ╚══════════════════════════════════════════════════════════════════════════════════════════════════════════╝

PROLOGUE:  I did as the handout instructed, and consulted the book's implementation of a simple implicit free list based dynamic memory allocator along with much of Chapter 9 as a point of reference.  I chose to use pointer arithmetic and not structs, just like the book and after asking the TAs permission, used several of the book's functions. 
__ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __
 
 STRUCTURE:
 * The heap is initialized to: | PADDING | DUMMY HEADER | DUMMY FOOTER | EPILOGUE |
 Where padding is 4 bytes, the header and footer are both 8 bytes, and the epilogue is 4. The sizing is so the heap stays aligned. The dummy ones are just initialized to a size of zero and allocated.

     *HEADERs contain the allocation and the size of the block.
     *FOOTERS also contain the allocation and the size of the block.

 * A free block looks like this:	| HEADER | PREV FREE | NEXT FREE | FORMER PAYLOAD | FOOTER |
     *PREV FREE and NEXT FREE are the addresses of the next & previous free blocks in the list.


 * An allocated block looks like this:	| HEADER |----------PAYLOAD----------| FOOTER |


 HANDLING OF FREE BLOCKS
 * A block is initialized as free, just meaning that the previous and next free pointers are set and the header and footer contain the block size and the lowest order byte  is 0 as a way to say "Hey I’m unallocated!"

 * When a block is freed, the formerly set header and footer’s size byte is reset to 0 instead of 1.  Nothing else gets changed. And while their actual location in memory may not correspond to their order in the list (especially after several coallesces and frees) the "front" of the free list is the most recently freed block. 


 MAINTAINING COMPACTION
 * In order to maximize utilization of space, I call coalesce immediately after I free a block, if I extend the heap, if I split a block, and if I place a free block into the free list. Basically, whenever I manipulate a free block, I check to see if it is touching any free blocks and if so, I join them and change headers.


CHECKER
I have three functions for checking
 * Function print_block, that will print a block’s status and next free block address if its free.
 * Function check_block, that checks a single block’s validity
 * Function mm_check_heap, that checks a heap’s prologue info along with every block in the heap and free list

Anytime an error occurs, the helper returns 0 and so when I called checker I’d look for it to equal 0, indicating an error and the print messages would show me what block and error I had encountered.

Initially, when I ran my code it was with mm_check_heap. I printed out every single block, however after debugging each function in my code and running the driver once with the checker to make sure there were no errors and I passed tests, I took out all calls to the checkers. If you do implement my checker(s), be warned that any block that is checked gets its status printed out.  This can be A LOT of blocks when running the driver, which I learned from experience. 
  
A single call to mm_check_heap checks the following and print corresponding error messages if an error occurs:
 * valid prologue header and footer for the heap
 * that each block in the free list is actually free and a valid block (using a for loop and call to check block)
 * for each block in the heap and the free list:
	* the block is properly aligned
	* the header and footer matches
	* the sizes are the same (I had an issue with this earlier hence the semi-redundant checking)
	* if the block is in bounds
	* if the block’s next free and previous free block are also in bounds

UNRESOLVED BUGS
There aren't any bugs from what I can tell. There is no overlapping of the blocks or mis-assignment of headers/ footers or any other issues that would mess up the memory allocator as far as the results of my checker and the driver indicate.


__ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ 

 ___     ___  __           __   __   ___  __    ___ 
|__  \_/  |  |__)  /\     /  ` |__) |__  |  \ |  |  
|___ / \  |  |  \ /~~\    \__, |  \ |___ |__/ |  |  

OPTIMIZATION PROCESS:
I changed the CHUNKSIZE macro to equal 1024 which was the largest number that resulted in my highest Perf index.


REALLOC
My function mm_realloc changes the size of an existing block based on the size argument, which is the new size of the block. 
     *If the new size is zero or less, just free the block. 
     *If the new size is the same as the block’s size, then return a pointer to the initial block. 
     *If the new size is less than the original size of the block, then change the size in the header and footer of the block to the new size and IF the rest of the block is at least MIN_BLOCK bytes, then I create a new free block with the remainder of the block to help with space utilization.
     *If the new size bigger than the old size,
	* If the next block is free and can be coalesced with the current block, and it will be a large enough size, then I merge the two and just return the original pointer.
	* If all else fails, I must call mm_malloc with the new size and copy all of the old data, then call free on the original block.

This is more efficient than the naive implementation, for I do not automatically jump to mm_malloc the minute the argument size is validated.  In my implementation, I minimize calls to mm_malloc so they’re only made when absolutely necessary!

__ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ 

Thank you to the TAs for providing such thorough instructions and tips! It helped a ton!

                       .========.
                     .-|        |-.
               .''-'`_.|__ __ __|._`'-''.
              / .--'`  |   [Q]  |  `'--. \
            .' /   _.--'''""""'''--._   \ '.
          /` .' .-'     _.----._     '-. '. `\
         |  /  /      .'  _  _  '.      \  \  |
         | |   |     /   `_  _`   \     |   | |
        /  /   '.   |    (o)(o)    |   .'   \  \
       |  |      '._|    .-""-.    |_.'      |  |
       |  \       / |     \  /     | \       /  |
       /  /      | / \    /\/\    / \ |      \  \
       | |      / |   '-.(    ).-'   | \      | |
       | |     | /       \`""`/       \ |     | |
       \  \    / |    _.-|    |-._    | \    /  /
        \  \  | /   .'   |    |   '.  \  |  /  /
         '. './ | .'    /      \    '. | \.' .'
           '._| \/                    \/ |_.'
             `'{`   ,              ,   `}'`
              {      }            {      }
              {     }              {     }
               {   }                {   }
                \,/                  \,/
                  '.                .'
                    '-.__      __.-'
                      {  _}""{_  }
                     /    \  /    \
                    /=/=|=|  |=|=\=\
                    \/\/\_/  \_/\/\/



  |   |                                   /        |                          | \ \  
  |   |   _` |  __ \   __ \   |   |      |   _` |  |  __ `__ \    _ \    __|  __|  | 
  ___ |  (   |  |   |  |   |  |   |      |  (   |  |  |   |   |  (   | \__ \  |    | 
 _|  _| \__,_|  .__/   .__/  \__, |      | \__,_| _| _|  _|  _| \___/  ____/ \__|  | 
               _|     _|     ____/      \_\                                      _/  

 __ __|  |                    |                 _)         _)                | 
    |    __ \    _` |  __ \   |  /   __|   _` |  | \ \   /  |  __ \    _` |  | 
    |    | | |  (   |  |   |    <  \__ \  (   |  |  \ \ /   |  |   |  (   | _| 
   _|   _| |_| \__,_| _|  _| _|\_\ ____/ \__, | _|   \_/   _| _|  _| \__, | _) 
                                         |___/                       |___/     

***********
Main Files:
***********

mm.{c,h}	
	Your solution malloc package. mm.c is the file that you
	will be handing in, and is the only file you should modify.

mdriver.c	
	The malloc driver that tests your mm.c file

short{1,2}-bal.rep
	Two tiny tracefiles to help you get started. 

Makefile	
	Builds the driver

**********************************
Other support files for the driver
**********************************

config.h	Configures the project driver
fsecs.{c,h}	Wrapper function for the different timer packages
clock.{c,h}	Routines for accessing the Pentium and Alpha cycle counters
fcyc.{c,h}	Timer functions based on cycle counters
ftimer.{c,h}	Object file; timer functions based on interval timers and gettimeofday()
memlib.{c,h}	Models the heap and sbrk function

*******************************
Building and running the driver
*******************************
To build the driver, type "make" to the shell.

To run the driver on a tiny test trace:
./mdriver -V -f short1-bal.rep

The -V option prints out helpful tracing and summary information.

To get a list of the driver flags:
./mdriver -h




