_HB_CLASS Address 
 function Address ( ... ) 
 STATIC s_oClass 
 LOCAL nScope, oClass, oInstance 
 IF s_oClass == NIL .AND. __clsLockDef( @s_oClass ) 
 BEGIN SEQUENCE 
 nScope := 1 
 ( ( nScope ) ) 
 oClass := iif( .F.,, HBClass():new( "Address", iif( .F., { }, { @HBObject() } ), @Address() ) ) 
 _HB_MEMBER New()
 oClass:AddMethod( "New", @Address_New(), nScope + iif( .F., 8, 0 ) + iif( .F., 256, 0 ) + iif( .F., 2048, 0 ) ) 
 _HB_MEMBER type()
 oClass:AddMethod( "type", @Address_type(), nScope + iif( .F., 8, 0 ) + iif( .F., 256, 0 ) + iif( .F., 2048, 0 ) )
 _HB_MEMBER { fields } 
 oClass:AddMultiData(,, nScope + iif( .F., 16, 0 ) + iif( .F., 256, 0 ) + iif( .F., 2048, 0 ), {"fields"}, .F. ) 
 _HB_MEMBER { union } 
 oClass:AddMultiData(,, nScope + iif( .F., 16, 0 ) + iif( .F., 256, 0 ) + iif( .F., 2048, 0 ), {"union"}, .F. ) 
 oClass:Create() 
 
 ALWAYS 
 __clsUnlockDef( @s_oClass, oClass ) 
 end 
 oInstance := oClass:Instance() 
 IF __objHasMsg( oInstance, "InitClass" ) 
 oInstance:InitClass( ... ) 
 END 
 RETURN oInstance 
 END 
 RETURN s_oClass:Instance() AS CLASS Address 
 static FUNCTION Address_New() 
 local Self AS CLASS Address := QSelf() AS CLASS Address
 ::union := {"fields" => { {"field" => "US", "type" => USAddress():new():type()}  } }
 return self

