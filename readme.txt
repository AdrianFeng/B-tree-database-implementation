project 2 Final version:


  This is a b+ tree implementation database project and it supports "load" and "select with condition" functionality. Simply use "make" to compile and then use ./bruinbase to run. Before any query, please load the data first. Do "LOAD tablename FROM 'filename' [ WITH INDEX ]" to load. For example, LOAD movie FROM 'movieData.del' and the "with index" will give an option to create a create index on Key attribute. After loading, then you can query, for example, "select count(*) from movie" , "select * from movie where key=3421" and "select * from movie where value='Die Another Day'".

