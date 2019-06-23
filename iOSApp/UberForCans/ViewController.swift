
import UIKit
import Particle_SDK
import MapKit
import PXGoogleDirections

public func debugLog(object: Any, functionName: String = #function, fileName: String = #file, lineNumber: Int = #line) {
    #if DEBUG
    let className = (fileName as NSString).lastPathComponent
    print("<\(className)> \(functionName) line \(lineNumber) | \(object)\n")
    #endif
}

class ViewController: UIViewController {
    
    @IBOutlet var multipleRoutesButton: UIButton!
    @IBOutlet var mapView: MKMapView!
    
    @IBAction func multipleRoute(_ sender: Any) {
        let directionsAPI = PXGoogleDirections(apiKey: "AIzaSyAdL0ELlJ26iLZUIE4zGHytTqdfS4hO6zg",
                                               from: PXLocation.coordinateLocation(loc.location!.coordinate),
                                               to: PXLocation.coordinateLocation(CLLocationCoordinate2D(latitude: loc.location!.coordinate.latitude - 0.004, longitude: loc.location!.coordinate.longitude - 0.005)))
        directionsAPI.optimizeWaypoints = true
        directionsAPI.waypoints = mapView.annotations.map( {PXLocation.coordinateLocation($0.coordinate)} )
        
        directionsAPI.calculateDirections({ response in
         switch response {
         case let .error(_, error):
         // Oops, something bad happened, see the error object for more information
            print(error)
         break
         case let .success(request, routes):
            if let rvc = self.storyboard?.instantiateViewController(withIdentifier: "Results") as? ResultsViewController {
                rvc.request = request
                rvc.results = routes
                self.present(rvc, animated: true, completion: nil)
            }
         break
         }
         })
        
    }
    
    
    let loc = CLLocationManager.init()
    var battery = 0.0
    var trash = 0.0
    var latitude = 0.0
    var longitude = 0.0
    
    
    override func viewDidAppear(_ animated: Bool) {
        loc.requestWhenInUseAuthorization()
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        ParticleCloud.sharedInstance().getDevices { (devices:[ParticleDevice]?, error:Error?) -> Void in
            if let _ = error {
                print("Check your internet connectivity")
            }
            else {
                if let myPhoton = devices?.first {
                    
                    let annotation = TrashAnnotation.init(device: myPhoton.name!, locationName: "Dolph's Trash Can", coordinate: CLLocationCoordinate2D(latitude: 0, longitude: 0))
                    
                    DispatchQueue.main.async( execute: {
                        self.mapView.addAnnotation(annotation)
                        self.mapView.userTrackingMode = .follow
                        //self.mapView.centerMapOnLocation(location: CLLocation(latitude: 37.7454, longitude: -122.4083))
                    })
                    
                    
                    myPhoton.handleAllEvents(handlers: [
                        "battery" : { (data: String) in
                            self.battery = Double(data)!
                        },
                        "trash" : { (data: String) in
                            self.trash = Double(data)!
                            self.mapView.view(for: annotation).map( {$0.image =  resizeImage(image: UIImage(named: self.trash >= 80 ? "FullTrashIcon" : "TrashIcon")!, targetSize: CGSize(width: 40, height: 40))} )
                        },
                        "latitude" : { (data: String) in
                            var copy = data
                            copy.removeLast()
                            self.latitude = (data.hasSuffix("N") ? 1.0 : -1.0) * Double(copy)!
                            annotation.coordinate.latitude = self.latitude
                        },
                        "longitude" : { (data) in
                            var copy = data
                            copy.removeLast()
                            self.longitude = (data.hasSuffix("E") ? 1.0 : -1.0) * Double(copy)!
                            annotation.coordinate.longitude = self.longitude
                        }])
                } else {
                    print("Couldn't find valid photon device")
                    abort()
                }
            }
        }
        
        mapView.delegate = self
        
        // Add dummy annotations
        if let l = loc.location {
            for i in 1...5 {
                let (dlat, dlong) = (Double.random(in: -0.01...0.01) , Double.random(in: -0.01...0.01))
                let annotation = TrashAnnotation.init(device: "Doesn't matter", locationName: "Dummy \(i)", coordinate: CLLocationCoordinate2D(latitude: l.coordinate.latitude + dlat, longitude: l.coordinate.longitude + dlong))
                mapView.addAnnotation(annotation)
            }
        }
        
    }
}

extension ViewController : MKMapViewDelegate {
    
    func mapView(_ mapView: MKMapView, viewFor annotation: MKAnnotation) -> MKAnnotationView? {
        // 2
        guard let annotation = annotation as? TrashAnnotation else { return nil }
        // 3
        let identifier = "marker"
        var view: MKMarkerAnnotationView
        // 4
        if let dequeuedView = mapView.dequeueReusableAnnotationView(withIdentifier: identifier)
            as? MKMarkerAnnotationView {
            dequeuedView.annotation = annotation
            view = dequeuedView
        } else {
            // 5
            view = MKMarkerAnnotationView(annotation: annotation, reuseIdentifier: identifier)
            view.image = resizeImage(image: UIImage(named: "TrashIcon")!, targetSize: CGSize(width: 40, height: 40))
            view.markerTintColor = .clear
            view.glyphImage = UIImage(named: "empty")
            
            view.canShowCallout = true
            
            let detailLabel = UILabel()
            detailLabel.numberOfLines = 0
            detailLabel.font = detailLabel.font.withSize(12)
            view.detailCalloutAccessoryView = detailLabel
            
            let mapsButton = UIButton(frame: CGRect(origin: CGPoint.zero,
                                                    size: CGSize(width: 30, height: 30)))
            mapsButton.setBackgroundImage(UIImage(named: "Maps-icon"), for: UIControl.State())
            view.rightCalloutAccessoryView = mapsButton
            let batteryView = BatteryIndicator(frame: CGRect(origin: CGPoint.zero,
                                                         size: CGSize(width: 30, height: 10)))
            
            view.leftCalloutAccessoryView = batteryView
            
            // Set up live updates for the real trash can
            if (annotation.device == "dolphPhoton1") {
                let updateData = { () in
                    detailLabel.text = "\(self.trash)% Full"
                    batteryView.precentCharged = self.battery
                    
                        annotation.coordinate = CLLocationCoordinate2DMake(self.latitude, self.longitude)
                    }
                NotificationCenter.default.addObserver(forName: .didReceiveData, object: nil, queue: .main, using: {(_) in updateData()})
                updateData()
            } else {
                // Set up fake values for dummy cans
                batteryView.precentCharged = Double.random(in: 0 ..< 100)
                let trashFullness = Double.random(in: 0 ..< 100)
                detailLabel.text = "\(Int(trashFullness))% Full"
                view.image = resizeImage(image: UIImage(named: trashFullness > 80 ? "FullTrashIcon" : "TrashIcon")!, targetSize: CGSize(width: 40, height: 40))
            }

        }
        return view
    }
    
    func mapView(_ mapView: MKMapView, annotationView view: MKAnnotationView,
                 calloutAccessoryControlTapped control: UIControl) {
        guard control is UIButton else { return }
        let location = view.annotation as! TrashAnnotation
        let launchOptions = [MKLaunchOptionsDirectionsModeKey:
            MKLaunchOptionsDirectionsModeDriving]
        let placemark = MKPlacemark(coordinate: location.coordinate)
        let mapItem = MKMapItem(placemark: placemark)
        mapItem.name = location.title!
        mapItem.openInMaps(launchOptions: launchOptions)
    }
}

extension MKMapView {
    func centerMapOnLocation(location: CLLocation) {
        let regionRadius: CLLocationDistance = 1000
        let coordinateRegion = MKCoordinateRegion(center: location.coordinate,
                                                  latitudinalMeters: regionRadius, longitudinalMeters: regionRadius)
        setRegion(coordinateRegion, animated: true)
    }
}

extension Notification.Name {
    static let didReceiveData = Notification.Name("didReceiveData")
}

extension ParticleDevice {
    
    func handleAllEvents(handlers: [String: (String) -> Void]) {
        subscribeToEvents(withPrefix: nil) { (event : ParticleEvent?, error : Error?) in
            if error != nil || event == nil {
                debugLog(object: "could not subscribe to events")
            } else {
                DispatchQueue.main.async( execute: {
                    guard let ev = event, let data = ev.data else { return }
                    debugLog(object: "got\(handlers[ev.event] == nil ? " unknown" : "") event \(ev.event) with data \(data)")
                    
                    handlers[ev.event].map { $0(data) }
                    NotificationCenter.default.post(name: .didReceiveData, object: nil)
                })
            }
        }
    }
    
    func fancyGetVariable<T>(name: String, completion: @escaping (T) -> Void) {
        self.getVariable(name, completion: { (result:Any?, error:Error?) -> Void in
            if let _ = error {
                print("Failed reading \(name) from device")
            }
            else {
                if let temp = result as? T {
                    completion(temp)
                } else {
                    print("Attempted to cast \(name) to incorrect type")
                    abort()
                }
            }
        })
    }
}

func resizeImage(image: UIImage, targetSize: CGSize) -> UIImage {
    let size = image.size
    
    let widthRatio  = targetSize.width  / size.width
    let heightRatio = targetSize.height / size.height
    
    // Figure out what our orientation is, and use that to form the rectangle
    var newSize: CGSize
    if(widthRatio > heightRatio) {
        newSize = CGSize(width: size.width * heightRatio, height: size.height * heightRatio)
    } else {
        newSize = CGSize(width: size.width * widthRatio,  height: size.height * widthRatio)
    }
    
    // This is the rect that we've calculated out and this is what is actually used below
    let rect = CGRect(x: 0, y: 0, width: newSize.width, height: newSize.height)
    
    // Actually do the resizing to the rect using the ImageContext stuff
    UIGraphicsBeginImageContextWithOptions(newSize, false, 3.0)
    image.draw(in: rect)
    let newImage = UIGraphicsGetImageFromCurrentImageContext()
    UIGraphicsEndImageContext()
    
    return newImage!
}
