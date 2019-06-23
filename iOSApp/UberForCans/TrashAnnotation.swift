
import Foundation
import MapKit

class TrashAnnotation: NSObject, MKAnnotation {
    dynamic var coordinate: CLLocationCoordinate2D
    var locationName: String
    var title: String?
    let device: String
    
    init(device: String, locationName: String, coordinate: CLLocationCoordinate2D) {
        self.locationName = locationName
        self.coordinate = coordinate
        self.title = locationName
        self.device = device
        
        super.init()
    }
}
