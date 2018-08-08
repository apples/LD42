
var LibraryEmberConfig = {
    ember_config_get: function() {
        var str = JSON.stringify(EmberConfig);
        var len = lengthBytesUTF8(str)+1;
        var mem = _malloc(len);
        stringToUTF8(str, mem, len);
        return mem;
    },
};

mergeInto(LibraryManager.library, LibraryEmberConfig);
