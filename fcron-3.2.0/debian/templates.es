Template: fcron/anacronwarn
Type: note
Description: Warning: interactions with anacron!
 If you have the anacron package in the 'removed' but not 'purged' state
 (i.e. anacron configuration files are still around the system), the fcron
 package will cause harmless side effects, such as reports of anacron being
 started at boot up.
 .
 DO NOT FILE BUGS AGAINST ANACRON IF YOU HAVE FCRON INSTALLED IN THE
 SYSTEM. They will be either reassigned to fcron to be summarily closed by
 me, or summarily closed by the anacron maintainer himself.
 .
 More information about this issue is available in
 /usr/share/doc/fcron/README.Debian
Description-es: Aviso sobre las interacciones con anacron.
 Si ha borrado el paquete anacron pero no lo ha purgado, sus ficheros de
 configuración aún se encuentran en el sistema y pueden provocar efectos
 colaterales en fcron y comportamientos extraños, como que parezca que se
 ha iniciado anacron al arrancar.
 .
 NO ENVÍE INFORMES DE ERROR CONTRA ANACRON SI TIENE FCRON INSTALADO EN EL
 SISTEMA. Serán reasignados a fcron y cerrados inmediatamente, o bien
 los cerrará el propio mantenedor de anacron.
 .
 Puede encontrar más información en /usr/share/doc/fcron/README.Debian.
