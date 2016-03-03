/**
 * List audio devices.
 * @author Raphael Medaer <rme@escaux.com>
 */

// Imports
var NodeCoreAudio = require('../../');

for (var i = 0; i < NodeCoreAudio.getNumDevices(); i++) {
	console.log(i, NodeCoreAudio.getDeviceName(i));
}
