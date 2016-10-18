# Makefile for relit

.PHONY: all man test clean bib run index test runRelit

relit.pdf all relit.aux-target relit.idx-target: relit.tex linesofcode.tex sedgewickeslinesofcode.tex lastwine.tex winelist.tex allLinesofcode.tex linesofrelit.tex e.txt relit.bbl relit.ind corelinesofcode.tex 
	./helpMake relit.pdf relit.aux relit.idx "pdflatex relit.tex"

man: relit.1
	man ./relit.1 
	
test: euler randomised-euler wine
	./euler
	./randomised-euler
	./wine
	
bib relit.bbl-target: relit.aux
	./helpMake relit.bbl "bibtex relit.aux"
	
index relit.ind-target: relit.idx
	./helpMake relit.ind "makeindex relit.idx"
	 
runRelit Makefile euler.c-target randomise-euler.c-target sedcommands-target euler.c-tagged.txt-target define-non-randomised-cycle-target wine.c-target: relit.tex relit  
	./helpMake euler.c-target randomise-euler.c-target sedcommands-target euler.c-tagged.txt-target define-non-randomised-cycle-target wine.c-target "./relit -u -v relit.tex"
	
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
	-rm -f allinesofcode.tex allLinesofcode.tex corelinesofcode.tex define-non-randomised-cycle e.txt euler euler.c relit.aux relit.blg relit.dvi relit.idx relit.ilg relit.log relit.spl relit.synctex.gz hello.c lastwine.tex linesofcode.tex linesofrelit.tex nameDemo.c randomised-euler randomised-euler.c relit relit-def.tex sedcommands sedgewickeslinesofcode.tex t TeX-mode-demo.tex uncom wine wine.c winelist.tex
	-rm -f *-target *-temp-*.tmp *-tagged.txt
