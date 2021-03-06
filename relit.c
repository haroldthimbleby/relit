// Relit - rverse literate programming
// October 2016 (c) Harold Thimbleby harold@thimbleby.net 
// Version 3.3
// *** parsing \relit{define name ., .} goes wrong because of backing up over a non-existent newline. 
		
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <regex.h>
#include <time.h>
#include <errno.h>

#define SUBSCRIPT_VALUE "(subscripted name)\n"
#define SUBSCRIPT_TYPE "subscripted name"
#define COUNTER_SYMBOL '#'

int verboseOption = 0, touchedFilesOption = 0, listFileNamesOption = 0, forceUpdateOption = 0, dOption = 0, allDefinitionsOption = 0, allNamesOption = 0, optionsOption = 0, subscriptListOption = 0, tagOption = 0;
char *defaultTag = "";
enum { UPDATE, UNCHANGED } touchWriting;
long touchLength;

struct dictionary // there aren't going to be many names, so a linked list will work fine
{	struct dictionary *next;
	char *type, *name, *value, *tag;
	int used, expanding, recursive, counter;
} *names = NULL;

char *safealloc(size_t n)
{	char *p = (char *) malloc(n);
	if( p == NULL )
	{	fprintf(stderr, "** run out of memory!\n");
		exit(0);
	}
	return p;
}

char *tagSeparator = ":";
void printTagUse(char *type, char *name, char *value, char *tag)
{	char *newlinep; int linecount = 1;
	while( (newlinep = index(value, '\n')) != NULL )
	{	fprintf(stdout, "%s%s%s%s%s%s%d%s%.*s\n", type, tagSeparator, name, tagSeparator, tag, tagSeparator, linecount++, tagSeparator, (int) (newlinep-value), value);
		value = newlinep+1;
	}
}

void checkDictionary()
{	for( struct dictionary *d = names; d != NULL; d = d->next )
    {	if( !d->used ) 
    		fprintf(stderr, "** unused %s %s\n%s", d->type, d->name, d->value);
		else if( d->recursive ) fprintf(stderr, "** recursive definition: %s %s\n%s", d->type, d->name, d->value);
		// printf %8s padding so generate and define align nicely
        else if( allDefinitionsOption ) fprintf(stderr, "%8s %s:\n%s", d->type, d->name, d->value);
        if( tagOption ) printTagUse(d->type, d->name, d->value, d->tag);
        if( !d->used || d->recursive || allDefinitionsOption )
        {	char *p = index(d->value, '\0'); // adding a newline looks much better, but is it misleading?
        	fprintf(stderr, "%s-----\n", p > d->value && p[-1] != '\n'? "\n----(end of line added above for neater report)": "");
        }
        else if( !allDefinitionsOption && allNamesOption ) fprintf(stderr, "%8s %s\n", d->type, d->name);
    }
}

void explainSubscripts()
{	int notes = 0;
	for( struct dictionary *d = names; d != NULL; d = d->next )
    	if( !strcmp(d->type, SUBSCRIPT_TYPE) )
		{	if( !notes++ && subscriptListOption ) fprintf(stderr, "Subscript notes:\n");
			if( d->counter > 1 && subscriptListOption ) fprintf(stderr, "     %s covers subscript range 1 to %d\n", d->name, d->counter);
			if( d->counter == 1 ) fprintf(stderr, "  ** %s covers subscript 1 only (only used once)\n", d->name);
		}
}

int counterName(char *name)
{	return index(name, COUNTER_SYMBOL) != NULL;
}

struct dictionary *rawlookup(char *name) // can lookup any name
{	for( struct dictionary *d = names; d != NULL; d = d->next )
		if( !strcmp(name, d->name) ) return d;
	return (struct dictionary *) NULL;
}

struct dictionary *dlookup(char *name) // it's an error to lookup a subscripted name
{	if( counterName(name) )
		fprintf(stderr, "** <%s> cannot be used with '%c' (only use '%c' in %%define or %%generate commands)\n", name, COUNTER_SYMBOL, COUNTER_SYMBOL);
	return rawlookup(name);
}

// the first time a subscripted name (eg name#) is defined, two definitions are actually made:
//     - name# is recursively created and initialised to 0
//     - name1 is created and defined
// subsequently, name# is convered to name2, name3, etc, and defined as usual
void define(char *type, char *name, char *value, char *tag)
{	if( strcmp(type, SUBSCRIPT_TYPE) && counterName(name) )
	{	struct dictionary *nameEntry = rawlookup(name);
		if( nameEntry == NULL ) // then first time, recurse to define the subscripted name (with #)
		{	define(SUBSCRIPT_TYPE, name, SUBSCRIPT_VALUE, tag);
			nameEntry = rawlookup(name);	
			nameEntry->used = 1; 
		}
		nameEntry->counter++;
		if( nameEntry->counter > 999 )
		{	fprintf(stderr, "** Oops. I never thought we'd need such big subscripts > 999. Please edit relit.c and recompile!!\n");
			exit(-1);
		}
		char *tname = safealloc(strlen(name)+1+3); // allows # counting up to 10^3-1 
		// now instantiate the subscript; replace COUNTER_SYMBOL (#) with the counter value
		int j = 0, countercounter = 0;
		for( int i = 0; name[i]; i++ )
		{	if( name[i] == COUNTER_SYMBOL )
			{   if( countercounter++ > 0 )
				{	fprintf(stderr, "** You cannot have more than one subscript '%c' character in %s\n", COUNTER_SYMBOL, name);
					break;
				}
				sprintf(&tname[j], "%d", nameEntry->counter);
			    while( tname[j] ) j++; // position over null at end of string
			}
			else tname[j++] = name[i];
		}
		tname[j] = '\0';
		// fprintf(stderr, "CHECK: define subscripted %s as %s [# should be instantiated as %d]\n", name, tname, nameEntry->counter);
		name = tname;
		// then fall through to define the instantiated subscripted name (with particular numbers)
	}
	struct dictionary *new = (struct dictionary *) safealloc(sizeof (struct dictionary)), **p;
	new->name = name;
	new->counter = 0;
	new->value = value;
	new->type = type;
	new->used = new->expanding = new->recursive = 0;
	new->tag = strcmp(tag, "none")? tag: "";
	for( p = &names; *p != NULL; p = &(*p)->next ) // insert sort
		if( strcmp(new->name, (*p)->name) < 0 ) break;
		else if( !strcmp(new->name, (*p)->name) )
		{	fprintf(stderr, "** %s %s: name redefined\n", type, name);
			return;
		}
	new->next = *p;
	*p = new;
}

int getOffset(char *type, char *where, char *name, int length, char *numeral)
{	int n = 0, sign = 1;
	if( length <= 0 ) return 0; // missing (optional) signed number treated as zero
	while( length > 0 && isspace(*numeral) ) length--, numeral++;
	if( *numeral == '-' ) sign = -1;
	else if( *numeral == '+' ) sign = 1;
	else 
	{	fprintf(stderr, "** %s %s: unrecognised %s number sign: %c (should be + or -)\n", type, name, where, *numeral);
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

#define REMAX 5
regex_t REkeyword, REreplace, REcheck, RErelit, RErelitCheck;
struct 
{	regex_t *compiled;
	char *regex;
} res[REMAX] =
{	{	&REkeyword, // match % keyword name /re/+n, /re/+n [, tag] OR % set-tag name
		"(%[ \t]*(generate|define)[ \t]+([[:alpha:]#][[:alnum:]#\\.-]*)[ \t]*(/[^/]*/|\\.)([ \t]*[+-][ \t]*[0-9]+)?[ \t]*,[ \t]*(/[^/]*/|\\.)([ \t]*[+-][ \t]*[0-9]+)?([ \t]*,[ \t]*([^\n]+))?[ \t]*\n)|(%[ \t]*set-tag[ \t]+([^\n \t]*)[ \t]*\n)"
	},
	{	&REreplace, // match <name>
		"<[ \t]*([[:alpha:]#][[:alnum:]#-]*)[ \t]*>"
	},
	{	&REcheck, // match the keyword alone - so if the REkeyword fails, this will match and hence print an error 
		"%[ \t]*(generate|define|set-tag)[^\n]*\n"
	},
	{	&RErelit,
		"\\\\relit(\\[[ \t]*([^] \t]*)[ \t]*\\])?\{[ \t]*(((generate|define)[ \t]+([[:alpha:]][[:alnum:]\\.-]*)[ \t]*(/[^/]*/|\\.)([ \t]*[+-][ \t]*[0-9]+)?[ \t]*,[ \t]*(/[^/]*/|\\.)([ \t]*[+-][ \t]*[0-9]+)?)|(set-tag[ \t]+([^}]+))|ends)[ \t]*\\}"
	},
	{	&RErelitCheck,
		"\\\\relit(\\[|\\{)[^\n]*\n"
	}
};

FILE *touchOpen(char *name)
{	FILE *fd;
	if( dOption ) return stderr; // any valid FILE* will do, as nothing will be done with it
	touchLength = 0L;
	if( forceUpdateOption || (fd = fopen(name, "r+")) == NULL ) // try creating the file
	{	touchWriting = UPDATE;
		return fopen(name, "w");
	}
	fseek(fd, 0L, SEEK_SET); // probably redundant
	touchWriting = UNCHANGED; // file exists
	return fd;
}

void touch(FILE *fd, char *s, long n) // write n bytes to file, but only update file if they are different
{	size_t bytesRead;
	long originalPosition = touchLength;
	if( dOption ) return;
	touchLength += n;
	if( touchWriting == UPDATE )
	{	fwrite(s, 1, n, fd);
		return;
	}
	char *bp = safealloc(n);
	bytesRead = fread(bp, 1, n, fd);
	touchWriting = bytesRead != n || strncmp(bp, s, n)? UPDATE: UNCHANGED;
	free(bp);
	if( touchWriting == UNCHANGED ) return; // they are the same; nothing to write
	if( fseek(fd, originalPosition, SEEK_SET) != 0 ) // backup the file pointer
		fprintf(stderr, "File seek failed - %s\n", strerror(errno));
	fwrite(s, 1, n, fd); // write over and update
}

void touchClose(FILE *fd, char *fileName) // trim file to actual length
{	if( listFileNamesOption ) printf("%s\n", fileName); 
	if( dOption ) return;
	fseek(fd, 0L, SEEK_END);
	if( ftell(fd) != touchLength ) 
	{	touchWriting = UPDATE; // in case new file is a shorter prefix (so not neeeding UPDATE so far)
		if( ftruncate(fileno(fd), touchLength) != 0 ) // truncate to correct length
			fprintf(stderr, "File truncate failed - %s\n", strerror(errno));
	}
	fclose(fd);
	if( !listFileNamesOption && (touchedFilesOption && touchWriting == UPDATE ) ) printf("%s\n", fileName); 
}

int recursivelyExpand(FILE *fd, char *filename, char *bp, char *tag, int tags)
{	regmatch_t REmatch[2];
	long offset;
	int usesTags = 0;
	if( tags ) recursivelyExpand(fd, filename, tag, "", 0);
	for( offset = 0; regexec(&REreplace, &bp[offset], 2, REmatch, 0) == 0; offset += REmatch[0].rm_eo )
	{	char *name = allocString(&bp[offset+REmatch[1].rm_so], REmatch[1].rm_eo-REmatch[1].rm_so);
		touch(fd, &bp[offset], (long) REmatch[0].rm_so);
		struct dictionary *d = dlookup(name);
		if( d == NULL ) fprintf(stderr, "** generate %s: no value defined for <%s>\n", filename, name);
		else if( d->expanding ) fprintf(stderr, "** generate %s: recursive call of <%s>\n", filename, name);
		else
		{	d->expanding = d->used = 1;
			usesTags |= recursivelyExpand(fd, filename, d->value, d->tag, tags);
			if( tags ) recursivelyExpand(fd, filename, tag, "", 0);
			d->expanding = 0;
			if( strlen(d->value) > 0 && d->value[strlen(d->value)-1] == '\n' && bp[offset+REmatch[0].rm_eo] == '\n' ) // if <name> and context have consecutive newlines, ignore one of them
				offset++;
		}
	}
	touch(fd, &bp[offset], (long) strlen(&bp[offset]));
	return usesTags || strcmp(tag, "");
}

void generateFiles()
{	FILE *fd;
	for( struct dictionary *d = names; d != NULL; d = d->next )
		if( !strcmp(d->type, "generate") )
		{	if( (fd = touchOpen(d->name)) != NULL )
			{	d->used = d->expanding = 1;
				int usesTags = recursivelyExpand(fd, d->name, d->value, d->tag, 0);
				d->expanding = 0;
				touchClose(fd, d->name);
				if( usesTags )
				{	char *tagSuffix = "-tagged.txt";
					char *tagfile = safealloc(strlen(d->name)+strlen(tagSuffix)+1);
					tagfile = strcat(strcpy(tagfile, d->name), tagSuffix);
					if( (fd = touchOpen(tagfile)) != NULL )
					{	d->expanding = 1;
						recursivelyExpand(fd, tagfile, d->value, d->tag, 1);
						d->expanding = 0;
						touchClose(fd, tagfile);
					}
					else 
						fprintf(stderr, "** generate %s: cannot create or open file \"%s\" for writing\n   Does this system information help: \"%s\"\n", d->name, tagfile, strerror(errno));
				}
			}
			else fprintf(stderr, "** generate %s: cannot create or open file \"%s\" for writing\n   Does this system information help: \"%s\"\n", d->name, d->name, strerror(errno));
		}
}

void saySearch(int dot, char *re, int delta, char *prefix, char *tag, char *terminator)
{	if( verboseOption )
	{	if( dot ) fprintf(stderr, ".");
		else fprintf(stderr, "/%s/", re);
		if( delta ) fprintf(stderr, "%s%d", delta < 0? "": "+", delta);
		fprintf(stderr, "%s%s%s", prefix, tag, terminator);
	}
}

int search(char *type, char *name, char *reName, char *bp, int offset, int dot, char *re, int delta, char *terminator)
{	regex_t compiled_re; 
	int startOffset = offset, startDelta = delta;
	regmatch_t REmatch[1];
	if( !dot )
	{	if( regcomp(&compiled_re, re, REG_EXTENDED) != 0 ) 
		{	fprintf(stderr, "** %s %s: re syntax error (%s search): /%s/\n", type, name, reName, re);
			return offset;
		}
		if( regexec(&compiled_re, &bp[offset], 1, REmatch, 0) == 0 ) offset = offset+REmatch[0].rm_eo-1;
		else 
		{	fprintf(stderr, "** %s %s: cannot find a match for (%s search): /%s/%s\n", type, name, reName, re, strcmp(reName, "end")? "": "\n   -- perhaps due to the start search skipping further than you expected?");
			return offset;
		}
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
	{	fprintf(stderr, "** %s %s: search for (%s search) failed: /%s/%s%d\n", type, name, reName, re, startDelta < 0? "": "+", startDelta);
		return startOffset;
	}
	return offset;
}

long process(char *bp, long offset, regmatch_t *REmatch, int setTag, int setTagIndex, int typeIndex, int nameIndex, int startDotIndex, int startDeltaIndex, int endDotIndex, int endDeltaIndex, int usesTag, int tagIndex)
{ 	char *type, *name, *start, *end, *tag;
	long startChar, endChar;
	int startDot, endDot, startDelta, endDelta;
	if( 0 ) // print RE matches - useful for getting REmatch indices right!
	{	for( int i = 0; i < 14; i++ ) 
			if( REmatch[i].rm_eo > 0 ) printf("%d = %.*s\n", i, (int) (REmatch[i].rm_eo-REmatch[i].rm_so), &bp[offset+REmatch[i].rm_so]);
			else printf("%d = NONE %d\n", i, (int) REmatch[i].rm_eo );
		//return offset+REmatch[0].rm_eo; // make progress
	}
	if( setTag ) // set-tag form
	{	defaultTag = allocString(&bp[offset+REmatch[setTagIndex].rm_so], REmatch[setTagIndex].rm_eo-REmatch[setTagIndex].rm_so);
		if( verboseOption ) fprintf(stderr, ":  set-tag %s\n", defaultTag);
		offset += REmatch[0].rm_eo; // skip past the matched %keyword line
		return offset;
	} 
	// else define or generate
	type = allocString(&bp[offset+REmatch[typeIndex].rm_so], REmatch[typeIndex].rm_eo-REmatch[typeIndex].rm_so);
	name = allocString(&bp[offset+REmatch[nameIndex].rm_so], REmatch[nameIndex].rm_eo-REmatch[nameIndex].rm_so);
	if( !(startDot = bp[offset+REmatch[startDotIndex].rm_so] == '.') ) start = allocString(&bp[offset+REmatch[startDotIndex].rm_so+1], REmatch[startDotIndex].rm_eo-REmatch[startDotIndex].rm_so-2);
	startDelta = getOffset(type, "start", name, (int) (REmatch[startDeltaIndex].rm_eo - REmatch[startDeltaIndex].rm_so), &bp[offset+REmatch[startDeltaIndex].rm_so]);
	if( !(endDot = bp[offset+REmatch[endDotIndex].rm_so] == '.') ) end = allocString(&bp[offset+REmatch[endDotIndex].rm_so+1], REmatch[endDotIndex].rm_eo-REmatch[endDotIndex].rm_so-2);
	endDelta = getOffset(type, "end", name, (int) (REmatch[endDeltaIndex].rm_eo - REmatch[endDeltaIndex].rm_so), &bp[offset+REmatch[endDeltaIndex].rm_so]);	
	if( verboseOption ) fprintf(stderr, ":    %s %s %s ", strcmp(type, "generate")? "    ": "", type, name); 
	tag = usesTag? allocString(&bp[offset+REmatch[tagIndex].rm_so], REmatch[tagIndex].rm_eo-REmatch[tagIndex].rm_so): defaultTag;
	offset += REmatch[0].rm_eo; // skip past the matched command
	saySearch(startDot, start, startDelta, "", "", ", ");
	saySearch(endDot, end, endDelta, strcmp(tag, "")? ", ":"", tag, "\n");
	startChar = search(type, name, "start", bp, offset, startDot, start, startDelta, ", ");
	endChar = search(type, name, "end", bp, startChar, endDot, end, endDelta, "");
	if( endChar >= startChar )
		define(type, name, allocString(&bp[startChar], endChar-startChar+1), tag);
	return offset;
}

struct structOption
{	char *option, **argument, *usage; int *optionFlag;
} options[] =
{	{"-a", NULL, "list all name and file definitions (overrides -n)", &allDefinitionsOption},
	{"-d", NULL, "do not generate any files", &dOption},
	{"-f", NULL, "touch (update) generated files", &forceUpdateOption},
	{"-g", NULL, "list names of all generated files to standard output (overrides -u)", &listFileNamesOption},
	{"-n", NULL, "list all names and files", &allNamesOption},
	{"-s", NULL, "list subscript information", &subscriptListOption},
	{"-t[separator]", &tagSeparator, "summarise all definitions with tags, line by line; designed for Unix filtering etc. Default separator is :", &tagOption},
	{"-u", NULL, "list names of updated files to standard output", &touchedFilesOption},
	{"-v", NULL, "verbose mode - trace command usage", &verboseOption},
	{"--", NULL, "treat all further parameters as filenames", &optionsOption},
};

int setOption(char *argvi)
{	if( !optionsOption )
	{	for( int o = 0; o < sizeof options/sizeof(struct structOption); o++ )
			if( !strncmp(options[o].option, argvi, 2) ) 
			{	if( strlen(argvi) != 2 )
				{	if( options[o].argument == NULL )
					{	fprintf(stderr, "** Option %s should not take a parameter\n", options[o].option);
						exit(0);
					} else 
						*options[o].argument = &argvi[2];
				}
				*options[o].optionFlag = 1;
				if( dOption && forceUpdateOption )
        		{	fprintf(stderr, "** Options -d and -f are incompatible!\n");
        			exit(0);
        		}
				return 1;
			}
	}
	return 0; // not an option
}

void usage(char *process)
{	fprintf(stderr, "** did not process any files\nUsage: %s ", process);
	for( int o = 0; o < sizeof options/sizeof(struct structOption); o++ )
		fprintf(stderr, "[%s] ", options[o].option);
	fprintf(stderr, "[name=value] files...\n");
	for( int o = 0; o < sizeof options/sizeof(struct structOption); o++ )
		fprintf(stderr, "       %s %s\n", options[o].option, options[o].usage);
	fprintf(stderr, "       name=value define name\n");
}

int main(int argc, char *argv[]) 
{	int fp, opened = 0, TeXMode = 0, nonTeXMode = 0;
	char *bp, *processedFileName;
	for( int i = 0; i < REMAX; i++ ) // set up the res
    {	if( regcomp(res[i].compiled, res[i].regex, REG_EXTENDED) != 0 )
    	{	fprintf(stderr, "** internal error: regcomp fail\n%s\n", res[i].regex);
    		int bra = 0, ket = 0;
			for( bp = res[i].regex; *bp; bp++ )
				if( *bp == '(' ) bra++;
				else if( *bp == ')' ) ket++;
			fprintf(stderr, "RE %d: bra = %d, ket = %d\n", i, bra, ket);
			exit(0);
		}
	}
	for( int i = 1; i < argc; i++ ) 
		if( setOption(argv[i]) ) continue;
		else if( !optionsOption && (bp = index(argv[i], '=')) != NULL ) define("command line name", allocString(argv[i], bp-argv[i]), &bp[1], "<command-line-tag>");
		else if( (fp = open(processedFileName = argv[i], O_RDONLY)) >= 0 )
		{	int thisfileTeXMode = 0, thisfilenonTeXmode = 0;
			struct stat stat_buf;
			fstat(fp, &stat_buf);
			bp = safealloc(3+stat_buf.st_size);
			if( read(fp, bp, stat_buf.st_size) != stat_buf.st_size )
			{	fprintf(stderr, "** cannot read from \"%s\" (maybe a permissions problem?)\n", processedFileName);
				continue;
			}
			bp[stat_buf.st_size] = (char) 0;
			opened = 1;
			regmatch_t REmatch[16];
			long offset = 0;
			if( verboseOption ) fprintf(stderr, ": File %s\n", processedFileName);
			// process any \relit[tag]{command} commands
			while( regexec(&RErelitCheck, &bp[offset], 14, REmatch, 0) == 0 ) // find everything of the form: \relit([|{) 
			{	long eo = REmatch[0].rm_eo, so = REmatch[0].rm_so;
				if( !thisfileTeXMode && verboseOption ) fprintf(stderr, ": '\\relit'-style commands...\n");
				thisfileTeXMode = TeXMode = 1;
				if( regexec(&RErelit, &bp[offset], 14, REmatch, 0) != 0 || REmatch[0].rm_so != so ) // it matched \relit, but not the rest...
				{	fprintf(stderr, "** Warning: line not in \\relit syntax (line ignored)\n%.*s", (int) (eo-so), &bp[offset+so]);
					offset += eo;
				} // else matched definitions of the form: \relit[tag]{keyword name re, re} OR \relit{set-tag text}
				else if( !strncmp(&bp[offset+REmatch[3].rm_so], "ends", 4) ) // skip \relit{ends}
					offset += eo;
				else if( REmatch[1].rm_eo > 0 && REmatch[11].rm_eo > 0 )
				{	fprintf(stderr, "** cannot use both [tag] and set-tag\n%.*s", (int) (eo-so), &bp[offset+so]);
					offset += eo;
				}
				else offset = process(bp, offset, REmatch, REmatch[11].rm_eo > 0, 12, 5, 6, 7, 8, 9, 10, REmatch[1].rm_eo > 0, 2);
			}
			offset = 0;
			while( regexec(&REcheck, &bp[offset], 14, REmatch, 0) == 0 ) // find everything of the form: % keyword... 
			{	long eo = REmatch[0].rm_eo, so = REmatch[0].rm_so;
				if( !thisfilenonTeXmode && verboseOption ) fprintf(stderr, ": '%%'-style commands...\n");
				thisfilenonTeXmode = nonTeXMode = 1;
				if( regexec(&REkeyword, &bp[offset], 14, REmatch, 0) != 0 || REmatch[0].rm_so != so ) // it matched % keyword, but not the rest...
				{	fprintf(stderr, "** Warning: line not in %% relit syntax (line ignored)\n%.*s", (int) (eo-so), &bp[offset+so]);
					offset += eo;
				} // else matched definitions of the form: % keyword name re, re [, tag] OR % set-tag text
				else offset = process(bp, offset, REmatch, REmatch[10].rm_eo > 0, 11, 2, 3, 4, 5, 6, 7, REmatch[8].rm_eo > 0, 9);
			}
			free(bp);
			close(fp);
		}
		else fprintf(stderr, "** cannot open \"%s\" %s\n", processedFileName,
				processedFileName[0] == '-'? "(and it isn't a recognised flag)": "");
	generateFiles();
	checkDictionary();
	explainSubscripts();
	if( TeXMode && nonTeXMode ) fprintf(stderr, "** Warning: mixes both '\\relit[]{}' and '%%' styles of relit command\n");
	if( !opened ) usage(argv[0]);
	return 0;
}
