========== tclral 0.7 ==========
New for Version 0.7:

1.  TEA compliant build procedures.

2.  Limited support for "virtual" relvars. Virtual relvars define a
    "view" that may be any relation valued expression. The expression
    is evaluated when the relvar value is requested. Note this is a
    very limited form of views in that no updating of the base relvars
    through the view is allowed.
