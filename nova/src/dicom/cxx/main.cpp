#include <iostream>
#include "dicom_image.h"
#include "api/dicom_api.h"
#include "lib/enum_utils.h"
#include "lib/logger.h"

int main() {
    nova::logger::init();
    auto handle = nova::api::new_dicom_handle();
    auto metaData = handle->get_image_metadata();
    // nova::dcm::dicom_image image("D:/repos/nova/nova-cli/input/CT-MONO2-16-brain.jls.dcm");
    // //nova::dcm::dicom_image image("D:/repos/nova/nova-cli/input/CTHead1.dcm");
    // //nova::dcm::dicom_image image("D:/repos/nova/nova-cli/input/HITTest1Fld27.CT.Spezial_01HIT_.3.1.2012.03.23.10.48.59.159.25131122.dcm");
    // auto result = image.load_image();
    // if (!result) {
    //     nova::logger::error("export failed: {}", result.error());
    // }
    nova::logger::info("exported finished");
    return 0;
}
