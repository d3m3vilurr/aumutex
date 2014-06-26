var os = require('os');

var aumutex = module.exports = require('./build/Release/aumutex');

exports.create = aumutex.create;
exports.enter = aumutex.enter;
exports.leave = aumutex.leave;
exports.close = aumutex.close;