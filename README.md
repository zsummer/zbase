
# Introduction:  
[![Build](https://github.com/zsummer/zbase/actions/workflows/cmake.yml/badge.svg)](https://github.com/zsummer/zbase/actions/workflows/cmake.yml)

from zbase

# Feature:  


#  Example  



# dist tree 
the dist dir auto release and commit from src by github action.   

```
dist
├── include
│   └── zbase
│       ├── LICENSE
│       ├── README.md
│       ├── VERSION
│       ├── zallocator.h
│       ├── zarray.h
│       ├── ...
│       └── zvector.h
│        
├── lib
│   └── zbase
└── doc
    └── zbase
```

# dist branch   
the dist branch auto merge  from ```[master]/dist``` to ```[dist]/```   



# How To Use  
1. copy [master]/dist to your project   
2. use [git subtree] into your project 
   * ```git subtree add --prefix=vender/zbase --squash -d  https://github.com/zsummer/zbase.git dist``` 
3. use [git submodule] into your project 
   * ```git submodule add -b dist https://github.com/zsummer/zbase.git vender/zbase```




 

# About The Author  
Author: YaweiZhang  
Mail: yawei.zhang@foxmail.com  
QQGROUP: 524700770  
GitHub: https://github.com/zsummer/zbase  

