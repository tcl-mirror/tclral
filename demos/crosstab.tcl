if 0 {
[Andrew Mangogna] 12 Feb 2007

After seeing [crosstab] and [crosstab again] by [Richard Suchenwirth]
I just couldn't resist stealing such a good idea and putting a
relational twist on it.
So, Richard, I hope you don't mind too much.

N.B. this contains some Tcl 8.5'isms.
}
 package require ral
 namespace import ::ral::*
 package require ralutil
 namespace import ::ralutil::*

 # crosstab relValue crossAttr ?attr1 attr2 ...?
 #
 # Generate a cross tabulation of "rel" for the "crossAttr" against the
 # variable number of attributes given. The "crossAttr" argument is the name of
 # an attribute of "relValue". The idea is to create new relation that contains all
 # the attributes in "args" plus a new attribute for each distinct value of
 # "crossAttr". The value of the new attributes is count of tuples that have
 # the corresponding value of "crossAttr".  Relationally, the "summarize"
 # command is used when computations are required across groups of tuples.
 proc crosstab {relValue crossAttr args} {
     # We start by projecting the attributes that will be retained
     # in the resulting relation.
     set subproj [relation project $relValue {expand}$args]
     # The strategy is to build up a summarize command on the fly, adding new
     # attributes. So we start with the constant part of the command.
     set sumCmd [list relation summarize $relValue $subproj r]

     # By projecting on the "crossAttr" we get the unique set of values
     # for that attribute since there are no duplicates in relations.
     set crossproj [relation project $relValue $crossAttr]
     # For each distince value of the "crossAttr" extend the relation with
     # a new attribute by the same name as the value and whose value is
     # the number of tuples which match the value.
     foreach val [lsort [relation list $crossproj]] {
	 set sumexpr [format\
	     {[relation cardinality [relation restrictwith $r {$%s == "%s"}]]}\
	     $crossAttr $val]
	 lappend sumCmd $val int $sumexpr
     }
     # Finally we want the total for all the "crossAttr" matches.
     lappend sumCmd Total int {[relation cardinality $r]}
     set ctab [eval $sumCmd]

     # At this point the relational algebra is over! The rest of this is just
     # to format some output. First we want to add totals across the bottom of
     # the tabular display. Technically, these totals are not part of the cross
     # tabulated relation since they represent different facts than the other
     # tuples in the relation. So we put the relation into a matrix and add in
     # the totals there. The matrix also serves as a convenient means of
     # formatting the output. TclRAL has support for moving relations into
     # matrices.

     set m [relation2matrix $ctab $args]
     # Get rid of the "data type" row in the header. It's ugly here.
     $m delete row 1
     # Add the row where the totals will go
     $m add rows 1
     $m set cell 0 end Total
     set colIndex [expr {[llength $args] - 1}]

     # The totals are easy to come by. They are just the summarization
     # of the original relation value over the "crossAttr".
     # Add them into the matrix.
     set totals [relation summarize $relValue $crossproj s\
	 Total int {[relation cardinality $s]}]
     relation foreach t $totals -ascending $crossAttr {
	 $m set cell [incr colIndex] end [relation extract $t Total]
     }
     # The grand total is just the number of tuples we started with.
     $m set cell end end [relation cardinality $relValue]

     # The report package seems a little complicated to use, but TclRAL
     # includes some support here too, since "relformat" uses the
     # report package to do formatting. A pre-supplied style helps here.
     ::report::report r [relation degree $ctab]\
	 style ::ral::relationAsTable {} 1
     # Put the totals line in the bottom caption of the report.
     r botdata set [r topdata get]
     r botcapsep set [r topcapsep get]
     r botcapsep enable
     r bcaption 1
     # Finally some text.
     set result [$m format 2string r]
     # Add a text caption to the output.
     set caption "Cross Tabulation of\
	 $crossAttr Against [join $args {, }]"
     append result $caption \n [string repeat "-" [string length $caption]]

     r destroy
     $m destroy

     return $result
 }

if 0 {
The original data set actually had two records labeled:

   * Jane;F;tennis

Since relations don't allow duplicates, we have to make a decision.
Either we must add another identifier in order to capture the idea
that someone may express interest in the same sport multiple times or we
deam this a data entry error and change "Jane" to something else.
I chose to do the latter in order to keep the output the same as Richard's
original.
So in this data set, "Alice" also prefers tennis.

I'm too lazy to write that part of the program that performs I/O.
Richard's original is very nice that way.
I'll just enter the data as a constant.
}

 set sportsData [relation create\
     {Name string Sex string Sport string}\
     {{Name Sex Sport}}\
     {Name John Sex M Sport soccer}\
     {Name Jane Sex F Sport tennis}\
     {Name Tom Sex M Sport football}\
     {Name Dick Sex M Sport soccer}\
     {Name Harry Sex M Sport tennis}\
     {Name Mary Sex F Sport baseball}\
     {Name Jeff Sex M Sport baseball}\
     {Name Alice Sex F Sport tennis}\
 ]

 puts [relformat $sportsData "Sports Data"]
if 0 {
 +======+======+========+
 |Name  |Sex   |Sport   |
 |string|string|string  |
 +======+======+========+
 |John  |M     |soccer  |
 |Jane  |F     |tennis  |
 |Tom   |M     |football|
 |Dick  |M     |soccer  |
 |Harry |M     |tennis  |
 |Mary  |F     |baseball|
 |Jeff  |M     |baseball|
 |Alice |F     |tennis  |
 +======+======+========+
 Sports Data
 -----------
}
 puts [crosstab $sportsData Sex Sport]
if 0 {
 +--------+-+-+-----+
 |Sport   |F|M|Total|
 +--------+-+-+-----+
 |baseball|1|1|2    |
 |football|0|1|1    |
 |soccer  |0|2|2    |
 |tennis  |2|1|3    |
 +--------+-+-+-----+
 |Total   |3|5|8    |
 +--------+-+-+-----+
 Cross Tabulation of Sex Against Sport
 -------------------------------------

It's also interesting to look at the transposition.
}
 puts [crosstab $sportsData Sport Sex]
if 0 {
 +-----+--------+--------+------+------+-----+
 |Sex  |baseball|football|soccer|tennis|Total|
 +-----+--------+--------+------+------+-----+
 |F    |1       |0       |0     |2     |3    |
 |M    |1       |1       |2     |1     |5    |
 +-----+--------+--------+------+------+-----+
 |Total|2       |1       |2     |3     |8    |
 +-----+--------+--------+------+------+-----+
 Cross Tabulation of Sport Against Sex
 -------------------------------------

And just for comparison, this is just the straight summarization across
the '''Sport''' and '''Sex''' attributes.
The major difference here is that the "0" rows are missing.
}

 puts [pipe {
     relation project $sportsData Sport Sex |
     relation summarize $sportsData ~ r Total int {[relation cardinality $r]} |
     relformat ~ "Totals over Sport and Sex" {Sport Sex}
 }]
if 0 {
 +========+======+-----+
 |Sport   |Sex   |Total|
 |string  |string|int  |
 +========+======+-----+
 |baseball|F     |1    |
 |baseball|M     |1    |
 |football|M     |1    |
 |soccer  |M     |2    |
 |tennis  |F     |2    |
 |tennis  |M     |1    |
 +========+======+-----+
 Totals over Sport and Sex
 -------------------------

----

See also [crosstab] and [crosstab again].
}
