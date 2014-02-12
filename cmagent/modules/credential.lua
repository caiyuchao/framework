-- Copyright (c) 2013 Elite Co., Ltd.
-- Author: Hong Shen <sh@ikwcn.com>

local os = os

local ok = os.execute("/usr/lib/auth/new_credential")
os.exit(ok / 256)
