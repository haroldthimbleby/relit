/* (c) H Thimbleby 2016
 * Strip out comments and blank lines from C, C++ and Java
 */
 
#include <stdio.h>

int initialBlanks = 0, skippednl = 0, indent = 0, startFile = 1;

void putMissingNLifAny()
{	if( skippednl ) 
	{	putchar('\n');
		skippednl = 0;
	}
}

void putnonblank(int c)
{	if( c == ' ' || c == '\t' )
	{	indent++;
		return;
	}
	if( c == '\n' ) 
	{	if( !startFile ) skippednl = 1; // ignore newlines at start of file
		indent = 0;
		return;
	}
	putMissingNLifAny();
	initialBlanks = startFile = 0;
	if( indent ) while( indent-- > 0 ) putchar(' ');
	indent = 0;
	putchar(c);
}

void skipline()
{	int c;
	while( (c = getchar()) != EOF )
		if( c == '\n' ) 
		{	putnonblank('\n');
			return;
		}
}

int endBlock()
{	int c;
	while( (c = getchar()) != EOF )
		if( c == '/' ) return 1;
		else return 0;
	return 0;
}

void skipblock()
{	int c;
	while( (c = getchar()) != EOF )
		if( c == '*' ) 
			if(	endBlock() ) return;
	return;
}

void afterslash() // skip //...\n or /*...*/
{	int c;
	while( (c = getchar()) != EOF )
	{	if( c == '/' ) skipline();
		else if( c == '*' ) skipblock();
		else
		{	putnonblank(c);
			return;
		}
	}
}

void afterquote() // skip "...."
{	int c;
	putnonblank('"');
	while( (c = getchar()) != EOF && c != '"' && c != '\n' )
	{	putnonblank(c);
		if( c == '\\' ) 
			if( (c = getchar()) != EOF ) putnonblank(c);
	}
	if( c != EOF ) putnonblank(c);
}

int main(int argc, char *argv[]) 
{	int c;
	while( (c = getchar()) != EOF )
		if( c == '/' ) afterslash();
		else if( c == '"' ) afterquote(); 
		else putnonblank(c);
	putMissingNLifAny();
	return 0;
}
