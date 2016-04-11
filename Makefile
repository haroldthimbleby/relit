# generated on Mon 11 Apr 2016 22:11:19 BST

all: euler randomised-euler winelist.tex lastwine.tex linesofcode.tex sedgewickeslinesofcode.tex e.txt allLinesofcode.tex linesofrelit.tex

euler.c euler.c-tagged.txt wine.c Makefile: eulerPaper.tex relit  
	./relit "date=`date`" command-line-tag="" eulerPaper.tex

euler: euler.c
	@echo " --" Check euler.c 
	cc euler.c -o euler
	./euler

randomised-euler: randomised-euler.c
	@echo " --" Check randomised-euler.c
	cc randomised-euler.c -o randomised-euler
	./randomised-euler

winelist.tex: wine
	./wine > winelist.tex 
	cat winelist.tex

wine: wine.c
	@echo " --" Check wine.c 
	cc wine.c -o wine
	
lastwine.tex: winelist.tex
	tail -n 1 winelist.tex > lastwine.tex
	
linesofcode.tex: euler.c uncom
	./uncom < euler.c | wc -l > t; echo Euler.c has `cat t` lines of code
	echo "\\\\newcount\\linesofcode \\linesofcode"=`cat t` > linesofcode.tex
	
sedgewickeslinesofcode.tex: java/DirectedEulerianCycle.java uncom
	./uncom < java/DirectedEulerianCycle.java | wc -l > t;
	echo "\\\\newcount\\linesofcode \\linesofcode"=`cat t` > sedgewickeslinesofcode.tex
	
e.txt: euler.c sedcommands
	sed -f sedcommands euler.c-tagged.txt > e.txt
	@echo Now ready to run LaTeX on eulerPaper.tex
	
allLinesofcode.tex: uncom 
	cat java/*.java | ./uncom | wc -l > t;
	echo "\\\\newcount\\linesofcode \\linesofcode"=`cat t` > allLinesofcode.tex
	
relit: relit.c
	cc relit.c -o relit
	
linesofrelit.tex: relit.c
	expr `cat relit.c|wc -l` / 10 "*" 10 > linesofrelit.tex
	
uncom: uncom.c
	cc uncom.c -o uncom
	
man: relit.1
	man ./relit.1
	
clean:
	-rm euler.c euler randomised-euler.c randomised-euler winelist.tex lastwine.tex linesofcode.tex sedgewickeslinesofcode.tex hello.c allLinesofcode.tex linesofrelit.tex e.txt uncom relit t *-tagged.txt wine.c wine
	-rm eulerPaper.aux eulerPaper.bbl eulerPaper.blg eulerPaper.log eulerPaper.synctex.gz
