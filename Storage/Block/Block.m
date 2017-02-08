
int main(int argc, char const *argv[])
{

	void (^ testBlock)() = ^ {
		
        int a;
        a = 1 + 1;
	};

	testBlock();

    
	return 0;
}
