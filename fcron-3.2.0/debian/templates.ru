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
Description-ru: Предупреждение: взаимодействия с anacron!
 Если вы имеете пакет anacron в состоянии 'removed', но не 'purged'
 (т.е. настроечные файлы anacron остоются в системе), то при пакете
 fcron могут возникать сторонние эффекты, такие как отчеты anacron при
 загрузке системы.
 .
 НЕ СЧИТАЙТЕ ЭТИ ФАЙЛЫ ОШИБКАМИ ANACRON, если у вас В СИСТЕМЕ
 УСТАНОВЛЕН FCRON. Чтобы избавиться от этой проблемы, они будут либо
 переназначены fcron мною, или закрыты самим сопровождающим anacron.
 .
 Больше информации об этой проблеме доступно в файле
 /usr/share/doc/fcron/README.Debian

