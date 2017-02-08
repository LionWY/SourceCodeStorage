//
//  Block2.m
//  
//
//  Created by FOODING on 17/2/76.
//
//


int global_val = 1;
static int static_global_val = 2;

int main()
{
    
    static int static_val = 3;
    
    
    __block int val = 10;
    
    void (^ testBlock)() = ^ {
        
        global_val *= 1;
        static_global_val *= 2;
        static_val *= 3;
        
        val = 1;
    };
    
    testBlock();
    
    
    
    return 0;
}
