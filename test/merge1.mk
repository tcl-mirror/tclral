JL   Bo0.9.0      Sun Mar 29 11:01:12 PDT 2009 Created by: "storeToMk merge1.mk" "±¢²Ô::CreateTrans ::DataElement ::MultiRefValue ::SubtypeRefValue ::Class ::__ral_systemids__ ::StateMap ::DomainOp ::State ::NormalTrans ::Transition ::InstanceMap ::ClassRef ::ClassStruct ::SingleRefValue ::Attribute ::SubSuperTraversal ::Instance ::Constructor ::Destructor ::AttrValue ::Domain ::EventMap ::SubtypeRef ::Operation ::SubtypeMap ::StateModel ::NamedInstance ::InstValue ::FinalState 	DomainId int ClassId int TransId int DomainId int ClassId int ElemId int ElemName string Line int DomainId int ClassId int InstId int ValueId int RefToInsts list DomainId int ClassId int InstId int ValueId int RefToClass string RefToInst string DomainId int ClassId int ClassName string Line int StorageSlots int PolyEvents list StorageClass string RelvarName string IdAttr string IdNum int DomainId int ClassId int StateName string StateNum int InitialState boolean FinalState boolean DomainId int DomainOpId int DomainOpName string Params list RetType string Line int Code string CodeLine int Comment string DomainId int ClassId int StateId int StateName string Params list Line int Code string CodeLine int DomainId int ClassId int TransId int StateName string DomainId int ClassId int TransId int EventName string NewState string Line int DomainId int ClassId int InstId int InstName string InstNum int DomainId int ClassId int ElemId int RefToClass string Multiple int Terminated boolean DomainId int ClassName string IAB string ODB string PDB string DomainId int ClassId int InstId int ValueId int RefToInst string DomainId int ClassId int ElemId int ElemDefault string DomainId int SubClassId int ElemId int SuperClassId int DomainId int ClassId int InstId int Line int DomainId int ClassId int Code string Line int DomainId int ClassId int Code string Line int DomainId int ClassId int InstId int ValueId int AttrValue string DomainId int DomainName string Line int IntfProlog string IntfPrologLine int IntfEpilog string IntfEpilogLine int ImplProlog string ImplPrologLine int ImplEpilog string ImplEpilogLine int DomainId int ClassId int EventName string EventNum int EventType string DomainId int ClassId int ElemId int RefType string RefToClass list DomainId int ClassId int OpId int OpName string Params list RetType string OpType string Line int Code string CodeLine int DomainId int ClassId int ElemId int ElemName string HierId int RefType string SubName string SubCode int DomainId int ClassId int DefTrans string InitialState string Line int DomainId int ClassId int InstId int InstName string DomainId int ClassId int InstId int ValueId int AttrName string Line int DomainId int ClassId int StateName string % = @ S h * _ | d 6 O @ V ? A 7 8 - . . A ¼ H C { i F 4 I * {DomainId ClassId TransId} {DomainId ClassId ElemId} {DomainId ClassId ElemName} {DomainId ClassId InstId ValueId} {DomainId ClassId InstId ValueId} {DomainId ClassId} {DomainId ClassName} {RelvarName IdAttr} {DomainId ClassId StateName} {DomainId ClassId StateNum} {DomainId DomainOpId} {DomainId DomainOpName} {DomainId ClassId StateId} {DomainId ClassId StateName} {DomainId ClassId TransId} {DomainId ClassId TransId} {DomainId ClassId InstId} {DomainId ClassId InstNum} {DomainId ClassId ElemId} {DomainId ClassName} {DomainId ClassId InstId ValueId} {DomainId ClassId ElemId} {DomainId SubClassId ElemId} {DomainId ClassId InstId} {DomainId ClassId} {DomainId ClassId} {DomainId ClassId InstId ValueId} DomainId DomainName {DomainId ClassId EventName} {DomainId ClassId EventNum EventType} {DomainId ClassId ElemId} {DomainId ClassId OpId} {DomainId ClassId OpName} {DomainId ClassId ElemId SubName} {DomainId ClassId ElemId SubCode} {DomainId ClassId ElemName SubName} {DomainId ClassId ElemName SubCode} {DomainId ClassId HierId SubName} {DomainId ClassId HierId SubCode} {DomainId ClassId} {DomainId ClassId InstId} {DomainId ClassId InstName} {DomainId ClassId InstId ValueId} {DomainId ClassId InstId AttrName} {DomainId ClassId StateName}  6 " " (  9 . 8   5   "      "  C  2 Ð  6 E  __CreateTrans_1 __DataElement_2 __MultiRefValue_3 __SubtypeRefValue_4 __Class_5 ____ral_systemids___6 __StateMap_7 __DomainOp_8 __State_9 __NormalTrans_10 __Transition_11 __InstanceMap_12 __ClassRef_13 __ClassStruct_14 __SingleRefValue_15 __Attribute_16 __SubSuperTraversal_17 __Instance_18 __Constructor_19 __Destructor_20 __AttrValue_21 __Domain_22 __EventMap_23 __SubtypeRef_24 __Operation_25 __SubtypeMap_26 __StateModel_27 __NamedInstance_28 __InstValue_29 __FinalState_30 

æó²¼Ã	ãÿ¼âÞ #üassociation ::R12 ::Instance {DomainId ClassId} * ::Class {DomainId ClassId} 1 association ::R13 ::InstValue {DomainId ClassId InstId} * ::Instance {DomainId ClassId InstId} 1 partition ::R14 ::InstValue {DomainId ClassId InstId ValueId} ::AttrValue {DomainId ClassId InstId ValueId} ::SingleRefValue {DomainId ClassId InstId ValueId} ::MultiRefValue {DomainId ClassId InstId ValueId} ::SubtypeRefValue {DomainId ClassId InstId ValueId} association ::R15 ::NamedInstance {DomainId ClassId InstId} ? ::Instance {DomainId ClassId InstId} 1 association ::R16 ::StateModel {DomainId ClassId InitialState} ? ::State {DomainId ClassId StateName} 1 association ::R17 ::FinalState {DomainId ClassId StateName} ? ::State {DomainId ClassId StateName} 1 association ::R18 ::FinalState {DomainId ClassId} * ::StateModel {DomainId ClassId} 1 association ::R19 ::Constructor {DomainId ClassId} ? ::Class {DomainId ClassId} 1 association ::R2 ::Class DomainId * ::Domain DomainId 1 association ::R20 ::Destructor {DomainId ClassId} ? ::Class {DomainId ClassId} 1 association ::R3 ::DataElement {DomainId ClassId} * ::Class {DomainId ClassId} 1 partition ::R4 ::DataElement {DomainId ClassId ElemId} ::Attribute {DomainId ClassId ElemId} ::ClassRef {DomainId ClassId ElemId} ::SubtypeRef {DomainId ClassId ElemId} association ::R5 ::StateModel {DomainId ClassId} ? ::Class {DomainId ClassId} 1 association ::R6 ::State {DomainId ClassId} * ::StateModel {DomainId ClassId} 1 association ::R7 ::Transition {DomainId ClassId} * ::StateModel {DomainId ClassId} 1 partition ::R8 ::Transition {DomainId ClassId TransId} ::NormalTrans {DomainId ClassId TransId} ::CreateTrans {DomainId ClassId TransId} association ::R9 ::DomainOp DomainId * ::Domain DomainId 1 association ::R10 ::Operation {DomainId ClassId} * ::Class {DomainId ClassId} 1 O a e h e V R 8 Q Q © P P U  ; P $»¤2Ö50061      1     4     33333350061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 ffffffff1 1 1 1 1 1 2 2 3 3 7 9 10 11 12 13 13 ªªªÿ1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 ªªþÿint a1 int a2 enum {A, B, C} foo R1 R2 R3 int superA R1 R2 R15 R5 int aa1 int aa2 int aa3 int aa4 int a int b 40 44 45 57 64 65 194 195 212 216 247 270 274 278 289 335 336 333DDDDDæ3µ4§4¤4Ëª4Ð4úî4ÿ5í¾5þ6¼50061 50061 f1 1 
    11 11     12 13     a b c a b c f6ê6ö6÷6û777777¢50061 50061 50061 50061 ff2 3 3 7 ª1 4 4 5 ª1 3 4 5 ªsub2 sub3 sub8 sub5 UUs2 ss3 ss8 ss5 CD7È7à7â7ê7ë7ó7ô7ü7ý888¢50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 ffffff1 2 3 4 5 6 7 8 9 10 11 12 13 ªªþc1 super1 super2 c2 sub1 sub2 sub3 sub4 sub5 sub6 sub7 sub8 c3 s7UUUU#36 193 211 226 228 237 246 260 269 273 277 288 334 CDDDDD$5 0 0 0 0 0 0 0 0 0 0 0 5 ªªªe1 e2 e1 e2 `     dynamic constant static static static static static static static static static static dynamic wwwwwÎ8Ð99¥9Ã¿9Ç:³::À:Ç:á:å:ñß:ø;×::Class ::DataElement ::State ::Transition ::Operation ::Instance ::InstValue ::DomainOp èØ¼¼ClassId ElemId StateId TransId OpId InstId ValueId DomainOpId xu¸13 17 9 16 2 16 15 1 ï¾Ù<<ê¾<î=¬=°=Å50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 fffff1 1 1 1 5 6 7 8 11 12 ªª. s1 s2 s3 s1 s1 s1 s1 s1 s1 þÿ0 1 2 3 0 0 0 0 0 0 ªª
0 0 0 1 1 1 1 1 1 1 ªª
0 1 0 0 0 0 0 0 0 0 ªª
¼=Þ>>>µ>¸>Õ>Ø>ì>ï???50061      1     Service      {int a} {char *s} int      348      
        struct c1* i ;
        PYCCA_selectOneInstWhere(i, c1, i->a1 == 27) ;

        printf("%d, %s\n", a, s) ;
        return a ;
      355      
    /*
     * The "Service operation is useful.
     */ 9?É?Ï?Õ?×?Ü?ä?ê?ü?ý@@@@AAA¢¹A¨Aá50061 50061 50061 50061 50061 50061 50061 50061 50061 ffff1 1 1 5 6 7 8 11 12 ªê1 2 3 4 5 6 7 8 9 ªªs1 s2 s3 s1 s1 s1 s1 s1 s1 ÿÿ{int p1} {int p2} {int p1} {int p2}        90 100 110 232 241 254 264 282 293 CDDD$
                /*
                 * This is ordinary "C" code that is passed through to
                 * the state action.
                 */
                printf("%d, %d\n", self->a1, rcvd_evt->p2) ;
             
                /*
                 * Use "Self" in comments to avoid the extraneous
                 * declaration of a variable.
                 */
                printf("%d, %d\n", rcvd_evt->p1, rcvd_evt->p2) ;
             
                // C++ comment style
                puts ("foo") ;
             
             
             
                struct sub5 *s = PYCCA_unionSubtype(self, R5, sub5) ;
             
             
             
             Þ æ R   T    93 103 111 233 242 255 265 283 294 CDDDT¶B¤BÚBßBóBöCCC¦¤C©CÍ£CÖCù°CþI®£IÀIã50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 fffffff1 1 1 5 5 6 6 7 7 8 8 11 11 12 12 ªªê?1 2 3 5 6 7 8 9 10 11 12 13 14 15 16 ªªÿ?s2 s3 s3 s1 s1 s1 s1 s1 s1 s1 s1 s1 s1 s1 s1 ÿÿÿ?ÚJ£Jý¢KK§¥K«KÐ­KÔL50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 ffffffff1 1 1 1 5 5 6 6 7 7 8 8 11 11 12 12 ªªªÿ1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 ªªþÿe2 e1 e2 e3 e1 e2 e1 e2 e1 e2 e1 e2 e1 e2 e1 e2 ÿÿÿÿs1 s1 IG s3 IG IG IG IG IG IG IG IG IG IG IG IG ÿÿÿÿ118 119 123 130 230 231 239 240 252 253 262 263 280 281 291 292 DDDDDDDDàL£M¤MM¯§M³MÚ°MÞN°NNÂÀNÆO50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 ffffffff1 1 1 1 1 1 2 3 4 4 4 5 6 7 9 12 ªªªê8 9 10 11 15 16 1 4 12 13 14 2 3 5 6 7 ú¯¿ªmyc1 this_c1 that_c2 a1 ss2 a b c s2 ss3 ss5 ss8 P C"CD0 1 2 3 4 5 0 0 0 1 2 0 0 0 0 0 ªªªªàOºP¡P¢PÃ§PÇPî±PòQ£ Q«QË50061 50061 50061 f1 1 1 *4 5 6 *c1 c2 c2 ?0 1 20 :false true false VQôRRRRRRRR R§R¨R¹50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 ffffffc1 super1 super2 c2 sub1 sub2 sub3 sub4 sub5 sub6 sub7 sub8 c3 s7UUUU3NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL UUUUUUNULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL UUUUUUNULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL UUUUUUÎRçSµ¿S¼SûÁTTÃÁTÊUÁUUÓ50061      1     10     10     myc1      UÿVVVVVVVV¢V§50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 fffff1 1 1 2 9 10 11 12 13 13 ªþ1 2 3 7 12 13 14 15 16 17 ªÿsizeof(int) + 32 A 10 10 10 10 0    ¼VÒWWW¬W¯WÉ¡WÌWí50061 50061 50061 50061 50061 50061 50061 50061 ffff5 6 7 8 11 12 9 10 ªï8 8 9 9 10 10 11 11 ªÿ2 2 3 3 3 3 7 7 ªª°XXÅXÉXÜXÞXòXôY50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 ffffffff2 5 6 3 7 9 12 1 1 1 1 4 4 4 1 1 ªºªª1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 ªªþÿ308 313 316 319 324 328 331 370 376 382 387 400 402 404 413 416 DDDDDDDDàY¤Z¡ZZ­§Z±ZØÀZÜ[50061      1     
            self->a1 = 0 ;
         %67     [Â[È[Î[Ð¥[Õ[ú[û[þ50061      1     
            self->a1 = 0 ;
         %71     \¡\§\­\¯¥\´\Ù\Ú\Ý50061 50061 50061 50061 50061 50061 50061 50061 ffff2 1 1 1 1 1 1 1 ªª1 8 9 9 10 11 15 16 ªÿ2 6 7 8 9 11 14 15 ªþ23 23 23 17 23 44 44 45 ÿÿ°]]°]´]Ä]Æ]Ú]Ü]ï]ñ^50061      foo      10     0     0     
    #include <stdio.h>
    #define INSTR_FUNC(s)  puts(s)
     @15     
    int
    main()
    {
        return 0 ;
    }
     820     ^°^¶^¼^À^Æ^É^Î^Ð^Õ^×À^Ü___ ¸_¥_Ý_Þ_á50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 fffffffff1 1 1 5 5 6 6 7 7 8 8 11 11 12 12 2 2 3 3 ªªê¿*e1 e2 e3 e1 e2 e1 e2 e1 e2 e1 e2 e1 e2 e1 e2 e1 e2 e1 e2 ÿÿÿÿ?0 1 2 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 ªªªª*event event event event event event event event event event event event event event event polyevent polyevent polyevent polyevent fffffff¦ª
ò`«aªa§aÑ¹aÖb¦bbºb¿cÁ50061 50061 50061 50061 ff2 3 3 7 ª8 9 10 11 úreference union union union jfsub1 sub2 sub3 sub4 sub7 sub8 sub5 sub6 ªªcñddddddd»¨d½då50061 50061 f1 1 
    1 2 
    c1op1 c1instop1 ¦{int a} {int b} {int d} {float a} int void Tclass instance 153 166 D
            struct c1 *it = Instance(c1, myc1) ;
            ClassRefVar(c1, i2) ;
            i2 = PYCCA_createInstance(c1, StateNumber(c1, s2)) ;
         
          
 157 170 Deeeee¢e¦e«e»¢e¼eÞeàeéeêeùeúf¨fg«g¯g·50061 50061 50061 50061 50061 50061 50061 50061 ffff2 2 3 3 3 3 7 7 ªª8 8 9 9 10 10 11 11 ªÿR1 R1 R2 R2 R15 R15 R5 R5 33D30 0 0 0 1 1 0 0 ªªreference reference union union union union union union ªfffsub1 sub2 sub3 sub4 sub7 sub8 sub5 sub6 UUUU0 1 0 1 0 1 0 1 ªª°hh±hµhÅhÇhÛhÝh÷hûi¸iiÅ¨iÉiñiõj50061 50061 50061 50061 50061 50061 50061 fff1 5 6 7 8 11 12 ª>IG CH CH CH CH CH CH ÿ?s3 s1 s1 s1 s1 s1 s1 ÿ?76 229 238 251 261 279 290 CDD´ªjÁjëjïjÿkkkk­k¯kÊ50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 ffffff2 6 3 7 9 12 1 1 1 4 4 4 ª®ª1 3 4 5 6 7 9 10 11 12 13 14 ªêÿa1 s2 ss2 ss3 ss5 ss8 myc1 this_c1 that_c2 a b c 3DD("Èkól»lÁlÚlÝlú±lým®50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 50061 fffffff2 2 3 3 7 1 1 1 1 1 1 1 1 1 1 ªªª*1 1 4 4 5 8 9 9 10 10 11 11 11 15 16 ªªÿ?1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 ªªþ?R1 int superA R2 R15 R5 int a1 int a1 int a2 int a1 R1 int a1 R2 R3 int a1 int a1 ³Csw77s309 310 320 321 325 371 377 378 383 384 388 389 394 414 417 DDDDDDDÚmÒn¬n´nÒ¥nÖnû¤nÿo£Òo§où¼pp½50061      1     s1     pñp÷pýpÿqqÖ__ral_version[Version_ral:S,Date_ral:S,Comment_ral:S],__ral_relvar[Name_ral:S,Heading_ral:S,Ids_ral:S,View_ral:S],__ral_constraint[Constraint_ral:S],__CreateTrans_1[DomainId:S,ClassId:S,TransId:S],__DataElement_2[DomainId:S,ClassId:S,ElemId:S,ElemName:S,Line:S],__MultiRefValue_3[DomainId:S,ClassId:S,InstId:S,ValueId:S,RefToInsts:S],__SubtypeRefValue_4[DomainId:S,ClassId:S,InstId:S,ValueId:S,RefToClass:S,RefToInst:S],__Class_5[DomainId:S,ClassId:S,ClassName:S,Line:S,StorageSlots:S,PolyEvents:S,StorageClass:S],____ral_systemids___6[RelvarName:S,IdAttr:S,IdNum:S],__StateMap_7[DomainId:S,ClassId:S,StateName:S,StateNum:S,InitialState:S,FinalState:S],__DomainOp_8[DomainId:S,DomainOpId:S,DomainOpName:S,Params:S,RetType:S,Line:S,Code:S,CodeLine:S,Comment:S],__State_9[DomainId:S,ClassId:S,StateId:S,StateName:S,Params:S,Line:S,Code:S,CodeLine:S],__NormalTrans_10[DomainId:S,ClassId:S,TransId:S,StateName:S],__Transition_11[DomainId:S,ClassId:S,TransId:S,EventName:S,NewState:S,Line:S],__InstanceMap_12[DomainId:S,ClassId:S,InstId:S,InstName:S,InstNum:S],__ClassRef_13[DomainId:S,ClassId:S,ElemId:S,RefToClass:S,Multiple:S,Terminated:S],__ClassStruct_14[DomainId:S,ClassName:S,IAB:S,ODB:S,PDB:S],__SingleRefValue_15[DomainId:S,ClassId:S,InstId:S,ValueId:S,RefToInst:S],__Attribute_16[DomainId:S,ClassId:S,ElemId:S,ElemDefault:S],__SubSuperTraversal_17[DomainId:S,SubClassId:S,ElemId:S,SuperClassId:S],__Instance_18[DomainId:S,ClassId:S,InstId:S,Line:S],__Constructor_19[DomainId:S,ClassId:S,Code:S,Line:S],__Destructor_20[DomainId:S,ClassId:S,Code:S,Line:S],__AttrValue_21[DomainId:S,ClassId:S,InstId:S,ValueId:S,AttrValue:S],__Domain_22[DomainId:S,DomainName:S,Line:S,IntfProlog:S,IntfPrologLine:S,IntfEpilog:S,IntfEpilogLine:S,ImplProlog:S,ImplPrologLine:S,ImplEpilog:S,ImplEpilogLine:S],__EventMap_23[DomainId:S,ClassId:S,EventName:S,EventNum:S,EventType:S],__SubtypeRef_24[DomainId:S,ClassId:S,ElemId:S,RefType:S,RefToClass:S],__Operation_25[DomainId:S,ClassId:S,OpId:S,OpName:S,Params:S,RetType:S,OpType:S,Line:S,Code:S,CodeLine:S],__SubtypeMap_26[DomainId:S,ClassId:S,ElemId:S,ElemName:S,HierId:S,RefType:S,SubName:S,SubCode:S],__StateModel_27[DomainId:S,ClassId:S,DefTrans:S,InitialState:S,Line:S],__NamedInstance_28[DomainId:S,ClassId:S,InstId:S,InstName:S],__InstValue_29[DomainId:S,ClassId:S,InstId:S,ValueId:S,AttrName:S,Line:S],__FinalState_30[DomainId:S,ClassId:S,StateName:S]Õ¡$2ú3¥6Å¥7£¬8¤³;Þ=Ç¬?ÂAâ»IèL¬O¥QÏ¬R»¥UÚ¥V­W÷Y[¤\\â¥^Å_æ¦cË¥dçÉg¸ºj¥kÎm´¬pÅq     B_ 	¼  8£