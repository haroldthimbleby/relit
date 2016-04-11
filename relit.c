// Inverted literate programming
// March 2016 (c) Harold Thimbleby harold@thimbleby.net 
// Version 2.1
		// BUG regex cannot use /, since that's the terminator
		// BUG if a line is not fully matched, it is ignored - we don't report any syntax error
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <regex.h>
#include <time.h>

struct dictionary // there aren't going to be many names, so a linked list will work fine
{	struct dictionary *next;
	char *type, *name, *value, *tag;
	int used, expanding, recursive;
} *names = NULL;

char *safealloc(size_t n)
{	char *p = (char *) malloc(n);
	if( p == NULL )
	{	fprintf(stderr, "** run out of memory!!\n");
		exit(0);
	}
	return p;
}

void checkDictionary(int everything)
{	for( struct dictionary *d = names; d != NULL; d = d->next )
    {	if( !d->used ) fprintf(stderr, "** unused %s %s\n%s", d->type, d->name, d->value);
		else if( d->recursive ) fprintf(stderr, "** recursive definition: %s %s\n%s", d->type, d->name, d->value);
        else if( everything ) fprintf(stderr, "%s %s:\n%s", d->type, d->name, d->value);
        if( !d->used || d->recursive || everything )
        {	char *p = index(d->value, '\0'); // adding a newline looks much better, but is it misleading?
        	fprintf(stderr, "%s-----\n", p > d->value && p[-1] != '\n'? "\n----(end of line added above for neater report)": "");
        }
    }
}

void define(char *type, char *name, char *value, char *tag)
{	struct dictionary *new = (struct dictionary *) safealloc(sizeof (struct dictionary)), **p;
	new->name = name;
	new->value = value;
	new->type = type;
	new->used = new->expanding = new->recursive = 0;
	new->tag = tag;
	for( p = &names; *p != NULL; p = &(*p)->next ) // insert sort
		if( strcmp(new->name, (*p)->name) < 0 ) break;
		else if( !strcmp(new->name, (*p)->name) )
		{	fprintf(stderr, "** %s %s: name redefined\n", type, name);
			return;
		}
	new->next = *p;
	*p = new;
}

struct dictionary *dlookup(char *name)
{	for( struct dictionary *d = names; d != NULL; d = d->next )
		if( !strcmp(name, d->name) ) return d;
	return (struct dictionary *) NULL;
}

int getOffset(char *type, char *where, char *name, int length, char *numeral)
{	int n = 0, sign = 1;
	if( length <= 0 ) return 0; // missing (optional) signed number treated as zero
	while( length > 0 && isspace(*numeral) ) length--, numeral++;
	if( *numeral == '-' ) sign = -1;
	else if( *numeral == '+' ) sign = 1;
	else 
	{	fprintf(stderr, "** %% %s %s: unrecognised %s number sign: %c (should be + or -)\n", type, name, where, *numeral);
		return 0;
	}
	length--, numeral++;
	while( length > 0 && isspace(*numeral) ) length--, numeral++;
	while( length > 0 && isdigit(*numeral) )
	{	n = n*10 + *numeral - (int) '0';
		length--, numeral++;
	}
	return n*sign;
}

char *allocString(char *bp, int length)
{   char *name = safealloc(length+1);
	strncpy(name, bp, length);
   	name[length] = (char) 0;
    return name;
}

regex_t REkeyword, REreplace, REcheck;

struct 
{	regex_t *compiled;
	char *regex;
} res[] =
{	{	&REkeyword, // match % keyword name /re/+n, /re/+n [, tag] OR % set-tag name
		"(%[ \t]*(generate|define)[ \t]+([[:alpha:]][[:alnum:].-]*)[ \t]*(/[^/]*/|\\.)([ \t]*[+-][ \t]*[0-9]+)?[ \t]*,[ \t]*(/[^/]*/|\\.)([ \t]*[+-][ \t]*[0-9]+)?([ \t]*,[ \t]*([^\n]+))?[ \t]*\n)|(%[ \t]*set-tag[ \t]+([^\n]+)[ \t]*\n)"
	},
	{	&REreplace, // match <name>
		"<[ \t]*([[:alpha:]][[:alnum:]-]*)[ \t]*>"
	},
	{	&REcheck,
		"%[ \t]*(generate|define|set-tag)[^\n]*\n"
	}
};

int recursivelyExpand(FILE *fd, char *filename, char *bp, char *tag, int tags)
{	regmatch_t REmatch[2];
	long offset;
	int usesTags = 0;
	if( tags ) recursivelyExpand(fd, filename, tag, "", 0);
	for( offset = 0; regexec(&REreplace, &bp[offset], 2, REmatch, 0) == 0; offset += REmatch[0].rm_eo )
	{	char *name = allocString(&bp[offset+REmatch[1].rm_so], REmatch[1].rm_eo-REmatch[1].rm_so);
		fprintf(fd, "%.*s", (int) REmatch[0].rm_so, &bp[offset]);
		struct dictionary *d = dlookup(name);
		if( d == NULL ) fprintf(stderr, "** %%generate %s: no value defined for <%s>\n", filename, name);
		else if( d->expanding ) fprintf(stderr, "** %%generate %s: recursive call of <%s>\n", filename, name);
		else
		{	d->expanding = d->used = 1;
			usesTags |= recursivelyExpand(fd, filename, d->value, d->tag, tags);
			if( tags ) recursivelyExpand(fd, filename, tag, "", 0);
			d->expanding = 0;
		}
	}
	fprintf(fd, "%s", &bp[offset]);
	return usesTags || strcmp(tag, "");
}

void generateFiles(int listFileNames)
{	FILE *fd;
	for( struct dictionary *d = names; d != NULL; d = d->next )
		if( !strcmp(d->type, "generate") )
		{	if( (fd = fopen(d->name, "w")) != NULL )
			{	d->used = d->expanding = 1;
				int usesTags = recursivelyExpand(fd, d->name, d->value, d->tag, 0);
				d->expanding = 0;
				fclose(fd);
				if( usesTags )
				{	char *tagSuffix = "-tagged.txt";
					char *tagfile = safealloc(strlen(d->name)+strlen(tagSuffix)+1);
					tagfile = strcat(strcpy(tagfile, d->name), tagSuffix);
					if( (fd = fopen(tagfile, "w")) != NULL )
					{	d->expanding = 1;
						recursivelyExpand(fd, tagfile, d->value, d->tag, 1);
						d->expanding = 0;
						fclose(fd);
					}
					else  fprintf(stderr, "** %%generate %s: cannot create or open file \"%s\" for writing\n", d->name, tagfile);
				}
				if( listFileNames ) printf("%s ", d->name);
			}
			else fprintf(stderr, "** %%generate %s: cannot create or open file \"%s\" for writing\n", d->name, d->name);
		}
	if( listFileNames ) printf("\n");
}

void saySearch(int dot, char *re, int delta, char *prefix, char *tag, char *terminator)
{	if( dot ) fprintf(stderr, ".");
	else fprintf(stderr, "/%s/", re);
	if( delta ) fprintf(stderr, "%s%d", delta < 0? "": "+", delta);
	fprintf(stderr, "%s%s%s", prefix, tag, terminator);
}

int search(char *type, char *name, char *reName, char *bp, int offset, int dot, char *re, int delta, char *terminator)
{	regex_t compiled_re; 
	int startOffset = offset, startDelta = delta;
	regmatch_t REmatch[1];
	if( !dot )
	{	if( regcomp(&compiled_re, re, REG_EXTENDED) != 0 ) 
		{	fprintf(stderr, "** %s %s: re syntax error (%s search): /%s/\n", type, name, reName, re);
			exit(0);
		}
		if( regexec(&compiled_re, &bp[offset], 1, REmatch, 0) == 0 ) offset = offset+REmatch[0].rm_eo-1;
		else fprintf(stderr, "** %s %s: cannot find a match for (%s search): /%s/\n", type, name, reName, re);
	}
	while( delta > 0 ) // advance a line
	{	while( bp[offset] && bp[offset] != '\n' ) offset++;
		delta--;
		if( bp[offset] ) offset++;
	}
	while( delta < 0 ) // go back a line
	{	while( offset > 0 && bp[offset] != '\n' ) offset--;
		delta++;
		if( offset > 0 ) offset--;
	}
	if( strcmp(reName, "start") ) while( bp[offset] && bp[offset] != '\n' ) offset++; // move to end of line
	else while( offset > 0 && bp[offset-1] != '\n' ) offset--; // move to start of line
	if( startOffset > offset ) 
	{	fprintf(stderr, "** %s %s: search for (%s search) went backwards: /%s/%s%d\n", type, name, reName, re, startDelta < 0? "": "+", startDelta);
		return startOffset;
	}
	return offset;
}

int main(int argc, char *argv[]) 
{	int fp, opened = 0, allDefinitions = 0, createdFiles = 0, options = 1;
	char *bp, *processedFileName, *tag, *defaultTag = "";
	for( int i = 0; i < 3; i++ )
    	if( regcomp(res[i].compiled, res[i].regex, REG_EXTENDED) != 0 )
    	{	fprintf(stderr, "** internal error: regcomp fail\n%s\n", res[i].regex);
    		exit(0);
    	}
	for( int i = 1; i < argc; i++ ) // process all options first
		if( options && !strcmp("-a", argv[i]) ) allDefinitions = 1;
		else if( options && !strcmp("-f", argv[i]) ) createdFiles = 1;
		else if( options && (bp = index(argv[i], '=')) != NULL ) define("command line name", allocString(argv[i], bp-argv[i]), &bp[1], "<command-line-tag>");
		else if( !strcmp("--", argv[i]) ) options = 0;
	options = 1;
	for( int i = 1; i < argc; i++ ) // then process files
		if( options && (!strcmp("-a", argv[i]) || !strcmp("-f", argv[i]) || index(argv[i], '=') != NULL) ) continue;
		else if( !strcmp("--", argv[i]) ) options = 0;
		else if( (fp = open(processedFileName = argv[i], O_RDONLY)) >= 0 )
		{	struct stat stat_buf;
			fstat(fp, &stat_buf);
			bp = safealloc(1+stat_buf.st_size);
			if( read(fp, bp, stat_buf.st_size) != stat_buf.st_size )
			{	fprintf(stderr, "** cannot read from \"%s\" (maybe a permissions problem?)\n", processedFileName);
				continue;
			}
			bp[1+stat_buf.st_size] = (char) 0;
			opened = 1;
			regmatch_t REmatch[16];
			long offset = 0;
			while( regexec(&REcheck, &bp[offset], 14, REmatch, 0) == 0 ) // find everything the form: % keyword 
			{	long eo = REmatch[0].rm_eo, so = REmatch[0].rm_so;
				if( regexec(&REkeyword, &bp[offset], 14, REmatch, 0) != 0 || REmatch[0].rm_so != so )
				{	fprintf(stderr, "** Warning: line may have a syntax error\n");
					fprintf(stderr, "%.*s", (int) (eo-so), &bp[offset+so]);
					fprintf(stderr, "\n");
					offset += eo;
					continue;	
				}
				// matched definitions of the form: % keyword name re, re [, tag] OR % set-tag text
				char *type, *name, *start, *end;
				long startChar, endChar;
				int startDot, endDot, startDelta, endDelta;
					/* for( int i = 0; i < 12; i++ ) // print RE matches - useful for getting REmatch index right
						  if( REmatch[i].rm_eo > 0 ) printf("%d = %.*s\n", i, (int) (REmatch[i].rm_eo-REmatch[i].rm_so), &bp[offset+REmatch[i].rm_so]);
						  else printf("%d = NONE %d\n", i, (int) REmatch[i].rm_eo );
					 */
				if( REmatch[10].rm_eo > 0 ) // % set-tag
				{	defaultTag = allocString(&bp[offset+REmatch[11].rm_so], REmatch[11].rm_eo-REmatch[11].rm_so);
					fprintf(stderr, ":  %% set-tag %s\n", defaultTag);
					offset += REmatch[0].rm_eo; // skip past the matched %keyword line
					continue;
				} // else % define or % generate	
				type = allocString(&bp[offset+REmatch[2].rm_so], REmatch[2].rm_eo-REmatch[2].rm_so);
				name = allocString(&bp[offset+REmatch[3].rm_so], REmatch[3].rm_eo-REmatch[3].rm_so);
				if( !(startDot = bp[offset+REmatch[4].rm_so] == '.') ) start = allocString(&bp[offset+REmatch[4].rm_so+1], REmatch[4].rm_eo-REmatch[4].rm_so-2);
				startDelta = getOffset(type, "start", name, (int) (REmatch[5].rm_eo - REmatch[5].rm_so), &bp[offset+REmatch[5].rm_so]);
				if( !(endDot = bp[offset+REmatch[6].rm_so] == '.') ) end = allocString(&bp[offset+REmatch[6].rm_so+1], REmatch[6].rm_eo-REmatch[6].rm_so-2);
				endDelta = getOffset(type, "end", name, (int) (REmatch[7].rm_eo - REmatch[7].rm_so), &bp[offset+REmatch[7].rm_so]);	
				fprintf(stderr, ":    %s%% %s %s ", strcmp(type, "generate")? "    ": "", type, name); 
				tag = REmatch[8].rm_eo > 0? allocString(&bp[offset+REmatch[9].rm_so], REmatch[9].rm_eo-REmatch[9].rm_so): defaultTag;
				offset += REmatch[0].rm_eo; // skip past the matched %keyword line
				saySearch(startDot, start, startDelta, "", "", ", ");
				saySearch(endDot, end, endDelta, ", ", tag, "\n");
				startChar = search(type, name, "start", bp, offset, startDot, start, startDelta, ", ");
				endChar = search(type, name, "end", bp, startChar, endDot, end, endDelta, "");
				define(type, name, allocString(&bp[startChar], endChar-startChar+1), tag);
			}
			if( offset == 0 ) fprintf(stderr, "** no %%define or %%generate found in %s (which is OK but strangely pointless)\n", processedFileName);
			free(bp);
			close(fp);
		}
		else fprintf(stderr, "** cannot open \"%s\"\n", processedFileName);
	generateFiles(createdFiles);
	checkDictionary(allDefinitions); 
	if( !opened ) fprintf(stderr, "** did not process any files\nUsage: %s [-a] [-f] [--] [name=value...] files...\n       -a print all name and file definitions\n       -f list generated files to standard output\n       name=value define new names\n       -- treat all further parameters as filenames\n", argv[0]);
	return 0;
}
