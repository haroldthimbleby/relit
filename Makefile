# generated on Mon 11 Apr 2016 16:35:49 BST

all euler.c euler.c-tagged.txt wine.c winelist.tex lastwine.tex linesofcode.tex segewiskeslinesofcode.tex e.txt: eulerPaper.tex relit uncom alllinesofcode.tex e.txt linesofrelit.tex sedcommands
	./relit "date=`date`" command-line-tag="" eulerPaper.tex
	@echo " --" Check euler.c 
	cc euler.c; ./a.out
	@echo " --" Check randomised-euler.c
	cc randomised-euler.c; ./a.out
	@echo " --" Check wine.c 
	cc wine.c; ./a.out > winelist.tex; cat winelist.tex
	tail -n 1 winelist.tex > lastwine.tex
	./uncom < euler.c | wc -l > t; echo Euler.c has `cat t` lines of code
	echo "\\\\newcount\\linesofcode \\linesofcode"=`cat t` > linesofcode.tex
	./uncom < java/DirectedEulerianCycle.java | wc -l > t;
	echo "\\\\newcount\\linesofcode \\linesofcode"=`cat t` > sedgewickeslinesofcode.tex
	sed -f sedcommands euler.c-tagged.txt > e.txt
	@echo Now ready to run LaTeX on eulerPaper.tex
	
alllinesofcode.tex: uncom 
	cat java/*.java | ./uncom | wc -l > t;
	echo "\\\\newcount\\linesofcode \\linesofcode"=`cat t` > alllinesofcode.tex
	
relit: relit.c
	cc relit.c -o relit
	
linesofrelit.tex: relit.c
	expr `cat relit.c|wc -l` / 10 "*" 10 > linesofrelit.tex
	
uncom: uncom.c
	cc uncom.c -o uncom
	
clean:
	-rm euler.c randomised-euler.c winelist.tex lastwine.tex linesofcode.tex sedgewickeslinesofcode.tex hello.c alllinesofcode.tex linesofrelit.tex e.txt a.out uncom relit t *-tagged.txt wine.c
	-rm eulerPaper.aux eulerPaper.bbl eulerPaper.blg eulerPaper.log eulerPaper.synctex.gz
