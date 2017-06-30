# Makefile for relit

.PHONY: all man test clean bib run index test runRelit zip

all relit.pdf relit.aux-target relit.idx-target: relit.tex linesofcode.tex sedgewickeslinesofcode.tex lastwine.tex winelist.tex allLinesofcode.tex linesofrelit.tex e.txt relit.ind-target corelinesofcode.tex diff.txt relit.bbl-target relit.idx-target TeX-mode-demo.tex
	./helpMake relit.pdf relit.aux relit.idx "pdflatex relit.tex"

man: relit.1
	man ./relit.1 
	
test: euler randomised-euler wine
	./euler
	./randomised-euler
	./wine
	
diff.txt: randomised-euler.c euler.c
	diff randomised-euler.c euler.c | grep -B 3 "walk a cycle" | sed "s/$$/\\\\\\\\/" > diff.txt
	
bib relit.bbl-target: relit.aux-target
	./helpMake relit.bbl "bibtex relit.aux"
	
index relit.ind-target: relit.idx-target
	./helpMake relit.ind "makeindex relit.idx"
	 
runRelit randomise-euler.c sedcommands euler.c-tagged.txt define-non-randomised-cycle wine.c euler.c: relit.tex relit  
	./relit relit.tex
	# echo "\\\\\\\\" `date` >> TeX-mode-demo.tex
	
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
	
zip elsevier.zip: # make elsevier.zip for Elsevier, does not include relit.bib
	echo zip does not include relit.bib, since we never delete that file
	zip elsevier.zip linesofcode.tex corelinesofcode.tex sedgewickeslinesofcode.tex allLinesofcode.tex winelist.tex lastwine.tex nameDemo.c linesofrelit.tex e.txt relit.ind diff.txt caption-1.tex caption-2.tex caption-3.tex caption-4.tex caption-5.tex caption-6.tex relit.aux relit.bbl relit.bib relit.tex
	
cleanlatex: # get rid of anything that Latex does not depend on
	-rm -f define-non-randomised-cycle euler euler.c relit.dvi relit.log relit.spl hello.c randomised-euler randomised-euler.c relit-def.tex sedcommands TeX-mode-demo.tex uncom wine wine.c a.out *-target *-tagged.txt elsevier.zip

clean: cleanlatex # leaves just the sources and the pdf file
	-rm -f allLinesofcode.tex corelinesofcode.tex define-non-randomised-cycle e.txt euler euler.c relit.aux relit.blg relit.dvi relit.idx relit.ilg relit.log relit.spl relit.synctex.gz hello.c lastwine.tex linesofcode.tex linesofrelit.tex nameDemo.c randomised-euler randomised-euler.c relit relit-def.tex sedcommands sedgewickeslinesofcode.tex t TeX-mode-demo.tex uncom wine wine.c winelist.tex a.out caption-6.tex caption-5.tex caption-4.tex caption-3.tex caption-2.tex caption-1.tex diff.txt relit.bbl relit.ind 
	-rm -f *-target *-temp-*.tmp *-tagged.txt
