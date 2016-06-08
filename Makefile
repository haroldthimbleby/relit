# Makefile for relit

.PHONY: all man test clean bib run index test runRelit

eulerPaper.pdf all eulerPaper.aux-target eulerPaper.idx-target: eulerPaper.tex linesofcode.tex sedgewickeslinesofcode.tex lastwine.tex winelist.tex allLinesofcode.tex linesofrelit.tex e.txt eulerPaper.bbl eulerPaper.ind corelinesofcode.tex 
	./helpMake eulerPaper.pdf eulerPaper.aux eulerPaper.idx "pdflatex eulerPaper.tex"

man: relit.1
	man ./relit.1 
	
test: euler randomised-euler wine
	./euler
	./randomised-euler
	./wine
	
bib eulerPaper.bbl-target: eulerPaper.aux
	./helpMake eulerPaper.bbl "bibtex eulerPaper.aux"
	
index eulerPaper.ind-target: eulerPaper.idx
	./helpMake eulerPaper.ind "makeindex eulerPaper.idx"
	 
runRelit Makefile euler.c-target randomise-euler.c-target sedcommands-target euler.c-tagged.txt-target define-non-randomised-cycle-target wine.c-target: eulerPaper.tex relit  
	./helpMake euler.c-target randomise-euler.c-target sedcommands-target euler.c-tagged.txt-target define-non-randomised-cycle-target wine.c-target "./relit -u -v eulerPaper.tex"
	
euler: euler.c
	cc euler.c -o euler
	./euler

randomised-euler: randomised-euler.c
	cc randomised-euler.c -o randomised-euler
	./randomised-euler 

winelist.tex: wine
	./wine > winelist.tex 
	
lastwine.tex: winelist.tex 
	tail -n 1 winelist.tex > lastwine.tex
	
linesofcode.tex: euler.c uncom
	./uncom < euler.c | wc -l > t
	@echo euler.c has `cat t` lines of code
	@echo "\\linesofcode"=`cat t` > linesofcode.tex
	@rm -f t
	
sedgewickeslinesofcode.tex: java/DirectedEulerianCycle.java uncom
	./uncom < java/DirectedEulerianCycle.java | wc -l > t
	@echo "\\linesofcode"=`cat t` > sedgewickeslinesofcode.tex
	@rm -f t
	
e.txt: euler.c sedcommands euler.c-tagged.txt
	sed -f sedcommands euler.c-tagged.txt > e.txt
	
allLinesofcode.tex: uncom 
	cat java/*.java | ./uncom | wc -l > t
	@echo "\\linesofcode"=`cat t` > allLinesofcode.tex
	@rm -f t
	
linesofrelit.tex: relit.c
	expr `cat relit.c|wc -l` / 10 "*" 10 > linesofrelit.tex
	
corelinesofcode.tex: define-non-randomised-cycle
	cat define-non-randomised-cycle | wc -l > t
	@echo "\\linesofcode"=`cat t` > corelinesofcode.tex
	@rm -f t
	
relit: relit.c
	cc relit.c -o relit
	
uncom: uncom.c
	cc uncom.c -o uncom
	
wine: wine.c
	cc wine.c -o wine 

clean: # leaves all the sources and the pdf file
	-rm -f allinesofcode.tex allLinesofcode.tex corelinesofcode.tex define-non-randomised-cycle e.txt euler euler.c eulerPaper.aux eulerPaper.blg eulerPaper.dvi eulerPaper.idx eulerPaper.ilg eulerPaper.log eulerPaper.synctex.gz hello.c lastwine.tex linesofcode.tex linesofrelit.tex randomised-euler randomised-euler.c relit relit-def.tex sedcommands sedgewickeslinesofcode.tex t TeX-mode-demo.tex uncom wine wine.c winelist.tex
	-rm -f *-target *-temp-*.tmp *-tagged.txt
