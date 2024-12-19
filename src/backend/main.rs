mod backend;

fn main() {
    let x = backend::init_client();
    println!("{:?}", x);
    backend::close_client(x);

    println!("hello");
}
